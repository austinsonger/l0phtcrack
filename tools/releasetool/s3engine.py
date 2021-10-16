from __future__ import print_function
import sys

def eprint(*args, **kwargs):
    print(*args, file=sys.stderr, **kwargs)

import os
from threading import Lock
import time
import datetime
import uuid
import traceback
import argparse
import urlparse
import urllib
from abc import ABCMeta, abstractmethod

try:
    import colorama
except ImportError:
    eprint("Please install colorama. `pip install colorama`")
    sys.exit(1)
from colorama import Fore, Back, Style

try:
    import boto
    from boto.s3.connection import S3Connection
    from boto.s3.multipart import MultiPartUpload, CompleteMultiPartUpload
    from boto.s3.key import Key
except ImportError:
    eprint("Please install boto. `pip install boto`")
    sys.exit(1)

try:
    from threadpool import ThreadPool, makeRequests, NoResultsPending
except ImportError:
    eprint("Please install threadpool. `pip install threadpool`")
    sys.exit(1)


# Get boto variables
LC7_RELEASE_ACCESS_KEY_ID = os.getenv("LC7_RELEASE_ACCESS_KEY_ID",None)
LC7_RELEASE_SECRET_ACCESS_KEY = os.getenv("LC7_RELEASE_SECRET_ACCESS_KEY",None)

########################################################################################################################

## S3 Utilities ##
def get_s3_connection():
    conn = boto.s3.connection.S3Connection(LC7_RELEASE_ACCESS_KEY_ID, LC7_RELEASE_SECRET_ACCESS_KEY, is_secure=False)
    if not conn:
        print("Unable to connect to S3")
        sys.exit(3)
    return conn

#
# Utilities
#

def info(x):
    return Fore.CYAN + Style.BRIGHT + x + Style.RESET_ALL


def error(x):
    return Fore.RED + Style.BRIGHT + x + Style.RESET_ALL


def warning(x):
    return Fore.YELLOW + Style.BRIGHT + x + Style.RESET_ALL


def success(x):
    return Fore.GREEN + Style.BRIGHT + x + Style.RESET_ALL


def stderr_print(x, cr=False, lf=True):
    x = x.encode("utf-8")
    if cr:
        x += "\r"
    if lf:
        x += "\n"
    sys.stderr.write(x)

########################################################################################################################

#
# Progress Callback Object
#

class S3EngineProgressCallback(object):
    __metaclass__ = ABCMeta

    @abstractmethod
    def started(self, action, args, id):
        pass

    @abstractmethod
    def progress(self, id,
                 part_current, part_total,
                 byte_current, byte_total,
                 complete_current, complete_total):
        pass

    @abstractmethod
    def finished(self, id):
        pass

    @abstractmethod
    def aborted(self, id):
        pass

#
# S3 Engine Exception Class

class S3EngineException(Exception):

    FAILED_TO_CONNECT = 1
    BUCKET_LOOKUP_FAILED = 2
    BUCKET_DOES_NOT_EXIST = 3
    MALFORMED_S3_URL = 4
    UNKNOWN_SIZE = 5
    NOT_SEEKABLE = 6
    INVALID_FILE = 7
    TRANSFER_ERROR = 8
    ABORTED_BY_USER = 9
    KEY_NOT_FOUND = 10

    def __init__(self, msg, error_code, inner_exception=None):
        super(S3EngineException, self).__init__(msg)
        self.error_code = error_code
        self.inner_exception = inner_exception

#
# Progress Managers
#

class _S3ProgressManager(object):

    def __init__(self, progress_callback, params):
        self.progress_callback = progress_callback
        self.params = params

        if self.progress_callback:
            self.progress_callback.started(params["action"],
                                           (params["from"], params["to"]),
                                           params["id"])
        self.did_complete = False

    def progress(self, current, total):

        try:
            if self.progress_callback:
                self.progress_callback.progress(None,
                                                1, 1,
                                                current, total,
                                                current, total)
            if current == total:
                self.did_complete = True
        except:
            raise



class _S3MultipartProgressManager(object):

    def __init__(self, progress_callback, params):
        self.progress_callback = progress_callback
        self.params = params
        self.lock = Lock()
        self.complete_current = 0
        self.part_current = [0] * params["part_total"]

        if self.progress_callback:
            self.progress_callback.started(params["action"],
                                           (params["from"], params["to"]),
                                           params["id"])
        self.did_complete = False

    def part_progress(self, part_number, current, total):

        try:
            self.lock.acquire()

            self.part_current[part_number-1] = current

            cc = 0
            for part in self.part_current:
                cc += part
            self.complete_current = cc

            if self.progress_callback:
                self.progress_callback.progress(self.params["id"],
                                                part_number, self.params["part_total"],
                                                current, total,
                                                self.complete_current, self.params["complete_total"])

            if self.complete_current == self.params["complete_total"]:
                self.did_complete = True
        except:
            print("Exception in part progress!")
            print(traceback.format_exc())
            raise

        finally:
            self.lock.release()


#
# Progress Callbacks
#

class _S3ProgressCallback(object):

    def __init__(self, progress_manager):
        self.progress_manager = progress_manager

    def __call__(self, current, total):
        self.progress_manager.progress(current, total)

class _S3MultipartProgressCallback(object):

    def __init__(self, progress_manager, part_number):
        self.progress_manager = progress_manager
        self.part_number = part_number

    def __call__(self, current, total):
        self.progress_manager.part_progress(self.part_number, current, total)


#
# S3 Engine Main Class
#


class S3Engine(object):

    MULTIPART_PART_SIZE = 10485760
    THREAD_POOL_SIZE = 8

    def __init__(self, conn, progress_callback=None):
        self.conn = conn
        self.progress_callback = progress_callback

    #
    # Internal Functions
    #

    def _get_bucket(self, bucket_name):
        try:
            bucket = self.conn.lookup(bucket_name)
        except Exception, e:
            raise S3EngineException(str(e), S3EngineException.BUCKET_LOOKUP_FAILED, e)

        if not bucket:
            raise S3EngineException("S3 bucket does not exist", S3EngineException.BUCKET_DOES_NOT_EXIST, None)

        return bucket


    def _split_s3url(self, s3url):

        try:
            p = urlparse.urlparse(s3url)
        except Exception, e:
            raise S3EngineException(str(e), S3EngineException.MALFORMED_S3_URL, e)
        if p.scheme.lower() != "s3":
            raise S3EngineException("Invalid scheme", S3EngineException.MALFORMED_S3_URL, None)
        if p.hostname is None:
            raise S3EngineException("Invalid bucket name", S3EngineException.MALFORMED_S3_URL, None)
        if p.port is not None:
            raise S3EngineException("Do not use port in S3 url", S3EngineException.MALFORMED_S3_URL, None)
        if p.params != "" or p.query != "" or p.fragment != "":
            raise S3EngineException("Do not use params, query, or fragment in S3 url", S3EngineException.MALFORMED_S3_URL, None)
        if p.username is not None or p.password is not None:
            raise S3EngineException("Do not use username or password in S3 url", S3EngineException.MALFORMED_S3_URL, None)

        return p.hostname, urllib.unquote(p.path.lstrip("/"))

    def _calculate_parts(self, filename, size, bucket_name, path, mpid):

        # Figure out how many parts
        partcount = size / S3Engine.MULTIPART_PART_SIZE
        leftover = size % S3Engine.MULTIPART_PART_SIZE

        # Divide leftovers by parts
        leftover_part = leftover / partcount
        #leftover_remainder = leftover % partcount      # should be equal to 'size' at end of loop

        # Get part size such that all threads finish about the same time
        partsize = S3Engine.MULTIPART_PART_SIZE + leftover_part

        parts = []
        start = 0
        part_number = 1
        while size >= partsize:

            parts.append({
                "filename": filename,
                "part_number": part_number,
                "start": start,
                "partsize": partsize,
                "bucket_name": bucket_name,
                "path": path,
                "mpid": mpid
            })

            size -= partsize
            start += partsize
            part_number += 1

        # Add remainder into last part
        parts[len(parts)-1]["partsize"] += size


        return parts


    def _download_part(self, args):

        filename = args["filename"]
        part_number = args["part_number"]
        start = args["start"]
        partsize = args["partsize"]
        bucket_name_from = args["bucket_name"]
        path_from = args["path"]
        mpid = args["mpid"]     # unused, we use path directly
        progress_manager = args["progress_manager"]

        def _download(retries):
            try:
                with open(filename, "r+b") as f:
                    # Boto is not thread safe here, so gotta get everything again
                    # in the thread context
                    bucket_from = self._get_bucket(bucket_name_from)
                    key = bucket_from.lookup(path_from)

                    headers = {'Range': "bytes=%d-%d" % (start, start+partsize-1)}

                    f.seek(start, os.SEEK_SET)
                    key.get_contents_to_file(f,
                                             headers=headers,
                                             num_cb=(partsize/65536)+1,
                                             cb=_S3MultipartProgressCallback(progress_manager, part_number))

                    progress_manager.part_progress(part_number, partsize, partsize)
            except Exception:
                if retries > 0:
                    _download(retries - 1)
                else:
                    raise

        _download(5)


    def _upload_part(self, args):

        filename = args["filename"]
        part_number = args["part_number"]
        start = args["start"]
        partsize = args["partsize"]
        bucket_name_to = args["bucket_name"]
        #path_to = args["path"]     # unused, mpid is what we send to
        mpid = args["mpid"]
        progress_manager = args["progress_manager"]

        def _upload(retries):
            try:
                with open(filename, "rb") as f:
                    # Boto is not thread safe here, so gotta get everything again
                    # in the thread context
                    bucket_to = self._get_bucket(bucket_name_to)
                    mps = bucket_to.list_multipart_uploads()
                    mpu = None
                    for mp in mps:
                        if mp.id == mpid:
                            mpu = mp
                            break

                    f.seek(start, os.SEEK_SET)
                    mpu.upload_part_from_file(f,
                                              part_number, # 1 based part number, not zero
                                              num_cb=(partsize/65536)+1,
                                              cb=_S3MultipartProgressCallback(progress_manager, part_number),
                                              size=partsize)

                    progress_manager.part_progress(part_number, partsize, partsize)
            except Exception:
                if retries > 0:
                    _upload(retries - 1)
                else:
                    raise

        _upload(5)


    #
    # Public Functions
    #

    def put(self, file_or_filename, s3url_to, parallel=False):

        if isinstance(file_or_filename, str) or isinstance(file_or_filename, unicode):
            try:
                fobj = open(file_or_filename, "rb")
                size = os.fstat(fobj.fileno()).st_size
            except Exception, e:
                raise S3EngineException("Can not get size of file", S3EngineException.INVALID_FILE, e)
        else:
            if parallel:
                raise S3EngineException("Parallel upload requires using filename", S3EngineException.REQUIRES_FILE, None)

            fobj = file_or_filename
            if hasattr(fobj, "seek") and hasattr(fobj, "tell") and (not hasattr(fobj, "seekable") or fobj.seekable()):
                curpos = fobj.tell()
                fobj.seek(0, os.SEEK_END)
                endpos = fobj.tell()
                fobj.seek(curpos, os.SEEK_SET)
                size = endpos - curpos
            else:
                raise S3EngineException("Size of input is unknown", S3EngineException.UNKNOWN_SIZE, None)

        # Get bucket
        (bucket_name_to, path_to) = self._split_s3url(s3url_to)
        bucket_to = self._get_bucket(bucket_name_to)

        # Cancel any previously existing multipart upload to this path
        self.abortmp(s3url_to)

        # Single or multipart upload
        if (not parallel) or (size < S3Engine.MULTIPART_PART_SIZE):
            # Single-part upload
            id = str(uuid.uuid4())
            try:
                key = bucket_to.new_key(path_to)


                # Create progress manager
                params = {
                    "id": id,
                    "action": "upload",
                    "from": file_or_filename,
                    "to": s3url_to,
                }
                progress_manager = _S3ProgressManager(self.progress_callback, params)

                key.set_contents_from_file(fobj, size=size, num_cb=(size/65536)+1, cb=_S3ProgressCallback(progress_manager))

                # Complete or abort the upload
                self.progress_callback.finished(id)

            except KeyboardInterrupt, e:
                if self.progress_callback:
                    self.progress_callback.aborted(id)
                raise S3EngineException(str(e), S3EngineException.ABORTED_BY_USER, e)

            except Exception, e:
                if self.progress_callback:
                    self.progress_callback.aborted(id)

                print(traceback.format_exc())

                raise S3EngineException, S3EngineException(str(e), S3EngineException.TRANSFER_ERROR, e), sys.exc_info()[2]
        else:
            # Multi-part upload
            mpu = None
            try:

                # Start new multipart upload
                mpu = bucket_to.initiate_multipart_upload(path_to)

                # Calculate parts
                parts = self._calculate_parts(file_or_filename, size, bucket_name_to, path_to, mpu.id)

                # Create progress manager
                params = {
                    "id": mpu.id,
                    "action": "multipart upload",
                    "from": file_or_filename,
                    "to": s3url_to,
                    "part_total": len(parts),
                    "complete_total": size
                }
                progress_manager = _S3MultipartProgressManager(self.progress_callback, params)

                # Add progress manager to all parts arguments
                for part in parts:
                    part["progress_manager"] = progress_manager

                # Create thread pool
                pool = ThreadPool(S3Engine.THREAD_POOL_SIZE)
                try:
                    reqs = makeRequests(self._upload_part, parts)

                    # Submit requests and wait
                    for req in reqs:
                        pool.putRequest(req)

                    try:
                        while True:
                            time.sleep(0.5)
                            pool.poll()
                    except NoResultsPending:
                        pass
                finally:
                    pool.dismissWorkers(S3Engine.THREAD_POOL_SIZE, do_join=True)

                # Complete or abort the upload
                if progress_manager.did_complete:
                    mpu.complete_upload()
                    if self.progress_callback:
                        self.progress_callback.finished(mpu.id)
                else:
                    mpu.cancel_upload()
                    if self.progress_callback:
                        self.progress_callback.aborted(mpu.id)

            except KeyboardInterrupt, e:

                if mpu:
                    mpu.cancel_upload()

                    if self.progress_callback:
                        self.progress_callback.aborted(mpu.id)

                raise S3EngineException(str(e), S3EngineException.ABORTED_BY_USER, e)

            except Exception, e:

                if mpu:
                    mpu.cancel_upload()
                    if self.progress_callback:
                        self.progress_callback.aborted(mpu.id)

                raise S3EngineException, S3EngineException(str(e), S3EngineException.TRANSFER_ERROR, e), sys.exc_info()[2]


    def get(self, s3url_from, file_or_filename, parallel=False):

        # Get bucket
        (bucket_name_from, path_from) = self._split_s3url(s3url_from)
        bucket_from = self._get_bucket(bucket_name_from)

        # Get size of download
        try:
            key = bucket_from.lookup(path_from)
        except Exception, e:
            raise S3EngineException("Unable to look up key (%s)" % str(e), S3EngineException.KEY_NOT_FOUND, e)

        if key is None or not key.exists():
            raise S3EngineException("Key not found", S3EngineException.KEY_NOT_FOUND, None)

        filesize = key.size
        if filesize is None:
            raise S3EngineException("Unable to get key size", S3EngineException.UNKNOWN_SIZE, None)

        # Touch target file
        fobj = None
        if isinstance(file_or_filename, str) or isinstance(file_or_filename, unicode):
            try:
                fd = os.open(file_or_filename, os.O_CREAT)
                os.close(fd)
                if (not parallel) or (filesize < S3Engine.MULTIPART_PART_SIZE):
                    fobj = open(file_or_filename, "r+b")
            except Exception, e:
                raise S3EngineException("Can't create file", S3EngineException.INVALID_FILE, e)
        else:
            if parallel:
                raise S3EngineException("Parallel download requires using filename", S3EngineException.REQUIRES_FILE, None)

            fobj = file_or_filename
            if hasattr(fobj, "seek") and hasattr(fobj, "tell") and hasattr(fobj, "truncate") and (not hasattr(fobj, "seekable") or fobj.seekable()):
                fobj.truncate(0)
            else:
                raise S3EngineException("File object must be seekable", S3EngineException.NOT_SEEKABLE, None)

        # Single or multipart download
        if (not parallel) or (filesize < S3Engine.MULTIPART_PART_SIZE):
            # Single part downlaod
            id = str(uuid.uuid4())
            try:

                # Create progress manager
                params = {
                    "id": id,
                    "action": "download",
                    "from": s3url_from,
                    "to": file_or_filename,
                }
                progress_manager = _S3ProgressManager(self.progress_callback, params)

                key.get_contents_to_file(fobj, num_cb=(filesize/65536)+1, cb=_S3ProgressCallback(progress_manager))

                # Complete the download
                self.progress_callback.finished(id)

            except KeyboardInterrupt, e:
                if self.progress_callback:
                    self.progress_callback.aborted(id)
                raise S3EngineException(str(e), S3EngineException.ABORTED_BY_USER, e)

            except Exception, e:
                if self.progress_callback:
                    self.progress_callback.aborted(id)

                print(traceback.format_exc())

                raise S3EngineException, S3EngineException(str(e), S3EngineException.TRANSFER_ERROR, e), sys.exc_info()[2]

        else:

            # Multi-part download
            try:

                id = str(uuid.uuid4())

                # Calculate parts
                parts = self._calculate_parts(file_or_filename, filesize, bucket_name_from, path_from, None)

                # Create progress manager
                params = {
                    "id": id,
                    "action": "multipart download",
                    "from": s3url_from,
                    "to": file_or_filename,
                    "part_total": len(parts),
                    "complete_total": filesize
                }
                progress_manager = _S3MultipartProgressManager(self.progress_callback, params)

                # Add progress manager to all parts arguments
                for part in parts:
                    part["progress_manager"] = progress_manager

                # Create thread pool
                pool = ThreadPool(S3Engine.THREAD_POOL_SIZE)
                try:
                    reqs = makeRequests(self._download_part, parts)

                    # Submit requests and wait
                    for req in reqs:
                        pool.putRequest(req)

                    try:
                        while True:
                            time.sleep(0.5)
                            pool.poll()
                    except NoResultsPending:
                        pass
                finally:
                    pool.dismissWorkers(S3Engine.THREAD_POOL_SIZE, do_join=True)

                # Complete or abort the upload
                if progress_manager.did_complete:
                    if self.progress_callback:
                        self.progress_callback.finished(id)
                else:
                    if self.progress_callback:
                        self.progress_callback.aborted(id)

            except KeyboardInterrupt, e:

                if self.progress_callback:
                    self.progress_callback.aborted(id)

                raise S3EngineException(str(e), S3EngineException.ABORTED_BY_USER, e)

            except Exception, e:

                if self.progress_callback:
                    self.progress_callback.aborted(id)

                raise S3EngineException, S3EngineException(str(e), S3EngineException.TRANSFER_ERROR, e), sys.exc_info()[2]

    def cp(self, s3url_from, s3url_to):
        """
        Copy one url to another in S3.
        :param s3url_from: A URL in the format s3://<bucket>/<object>
        :param s3url_from: A URL in the format s3://<bucket>/<object>
        """

        (bucket_name_from, path_from) = self._split_s3url(s3url_from)
        bucket_from = self._get_bucket(bucket_name_from)

        (bucket_name_to, path_to) = self._split_s3url(s3url_to)
        #bucket_to = self._get_bucket(bucket_name_to)

        id = str(uuid.uuid4())

        params = {
            "id": id,
            "action": "copy",
            "from": s3url_from,
            "to": s3url_to,
        }

        progress_manager = _S3ProgressManager(self.progress_callback, params)

        try:
            key_from = bucket_from.lookup(path_from)
            if not key_from or not key_from.exists():
                self.progress_callback.aborted(id)
                raise S3EngineException("Source key does not exist", S3EngineException.TRANSFER_ERROR, None)

            key_to = key_from.copy(bucket_name_to, path_to)
            if not key_to or not key_to.exists():
                self.progress_callback.aborted(id)
                raise S3EngineException("Could not copy key", S3EngineException.TRANSFER_ERROR, None)

            self.progress_callback.finished(id)
        except:
            self.progress_callback.aborted(id)
            raise


    def ls(self, s3url, recursive=False):
        """
        List all contents of an S3 URL.
        'recursive' list all keys/paths under a prefix.
        :param s3url: A URL in the format s3://<bucket>/<object>
        :return: the list of all sub-entries of this path
        """

        (bucket_name, path) = self._split_s3url(s3url)

        bucket = self._get_bucket(bucket_name)

        if path.endswith("/"):
            keys = [k.name for k in bucket.list(path, "/" if not recursive else "")]
            if len(keys) != 0:
                return keys
            return None

        keys = [k.name for k in bucket.list(path+"/", "/" if not recursive else "")]
        if len(keys) != 0:
            return keys

        keys = [k.name for k in bucket.list(path, "/" if not recursive else "")]
        if len(keys) != 0:
            return keys

        return None


    def rm(self, s3url, recursive=False):
        """
        Remove all contents of an S3 URL.
        'recursive' removes all keys/paths under a prefix.
        Without 'recursive', it will remove all files in the current folder except for subfolders and their files.
        'recursive' has no effect on S3 URLs that are pointing to a single file.
        :param s3url: A URL in the format s3://<bucket>/<object>
        :return: the list of all keys remaining out of the set
        """

        keys = self.ls(s3url, recursive)
        if keys is None:
            return

        (bucket_name, path) = self._split_s3url(s3url)
        bucket = self._get_bucket(bucket_name)

        mdr = bucket.delete_keys(keys)

        return ["s3://"+bucket+"/"+k.name for k in mdr.errors]


    def listmp(self, s3url):
        """
        List all multipart uploads associated with a particular url prefix
        :param s3url: A URL in the format s3://<bucket>/<object>
        :return: a list of (keyname, id) pairs that correspond to the multipart uploads in progress
        with the keys that start with the s3url path.
        """

        (bucket_name, path) = self._split_s3url(s3url)

        bucket = self._get_bucket(bucket_name)

        mps = bucket.list_multipart_uploads()

        mps = [("s3://%s/%s" % (bucket_name, mp.key_name), mp.id) for mp in mps if mp.key_name.startswith(path)]

        return mps


    def abortmp(self, s3url, id=None):
        """
        Cancel all multipart uploads associated with a particular url prefix
        :param s3url: A URL in the format s3://<bucket>/<object>
        :param id: A multipart ID as returned by 'listmp' of a specific multipart to cancel
        :return: the count of multipart uploads cancelled
        """

        (bucket_name, path) = self._split_s3url(s3url)

        bucket = self._get_bucket(bucket_name)

        cancelled = 0
        mps = bucket.list_multipart_uploads()
        for mp in mps:
            if mp.key_name.startswith(path) and (id is None or mp.id == id):
                mp.cancel_upload()
                cancelled += 1

        return cancelled


########################################################################################################################


#
# S3 Engine Command Line Tool
#

class ConsoleProgressCallback(S3EngineProgressCallback):

    def __init__(self):
        self.action = None
        self.args = None
        self.id = None
        self.parts = []
        self.complete_current = 0
        self.complete_total = 0
        self.start_time = None
        pass

    def started(self, action, args, id):
        self.action = action
        self.args = args
        self.id = id

        stderr_print(info("Starting %s for id %s" % (self.action, self.id)))

        if self.action == "upload" or self.action == "multipart upload":
            stderr_print("%s -> %s" % (self.args[0], self.args[1]))
        elif self.action == "download" or self.action == "multipart download":
            stderr_print("%s -> %s" % (self.args[0], self.args[1]))
        elif self.action == "copy":
            stderr_print("%s -> %s" % (self.args[0], self.args[1]))

        self.start_time = datetime.datetime.now()

    def progress(self, id,
                 part_current, part_total,
                 byte_current, byte_total,
                 complete_current, complete_total):

        if len(self.parts) != part_total:
            self.parts = [None]*part_total

        self.parts[part_current-1] = {"current": byte_current, "total": byte_total}
        self.complete_current = complete_current
        self.complete_total = complete_total

        self.display_progress()

    def calc_bps(self):
        elapsed_time = datetime.datetime.now() - self.start_time
        fltsecs = float(elapsed_time.days)*3600*24 + \
            float(elapsed_time.seconds) + \
            float(elapsed_time.microseconds)/1000000.0

        bps = float(self.complete_current) / fltsecs

        suffix = "bps"
        if bps > 1024.0:
            bps /= 1024.0
            suffix = "Kbps"
        if bps > 1024.0:
            bps /= 1024.0
            suffix = "Mbps"
        if bps > 1024.0:
            bps /= 1024.0
            suffix = "Gbps"

        return "%.2f%s" % (bps, suffix)

    def finished(self, id):

        stderr_print(success("\nOperation finished for id '%s'" % str(id)))

        if self.action == "get" or self.action == "put":
            bps = self.calc_bps()
            stderr_print("Transfer rate: %s" % bps)

    def aborted(self, id):
        stderr_print(error("\nOperation aborted for id '%s'" % str(id)))

    def display_progress(self):
        bps = self.calc_bps()
        stderr_print("Progress: %d/%d (%.2f%%) %s                    " % (self.complete_current, self.complete_total, (100.0 * self.complete_current / self.complete_total), bps), lf=False, cr=True)


def cmd_put(args):
    s3 = S3Engine(get_s3_connection(),progress_callback=ConsoleProgressCallback())
    s3.put(args.file, args.url, parallel=args.parallel)


def cmd_get(args):
    s3 = S3Engine(get_s3_connection(),progress_callback=ConsoleProgressCallback())
    s3.get(args.url, args.file, parallel=args.parallel)


def cmd_cp(args):
    s3 = S3Engine(get_s3_connection(),progress_callback=ConsoleProgressCallback())
    s3.cp(args.url_from, args.url_to, parallel=args.parallel)


def cmd_ls(args):
    s3 = S3Engine(get_s3_connection(),progress_callback=ConsoleProgressCallback())
    keys = s3.ls(args.url, args.recursive)
    if not keys or len(keys) == 0:
        stderr_print(warning("No results."))
    else:
        stderr_print(success("Results:"))
        for key in keys:
            print(key)


def cmd_rm(args):
    s3 = S3Engine(get_s3_connection(),progress_callback=ConsoleProgressCallback())
    keys = s3.rm(args.url, args.recursive)
    if not keys or len(keys) == 0:
        stderr_print(success("All specified keys deleted."))
    else:
        stderr_print(warning("Did not delete some keys:"))
        for key in keys:
            print(key)


def cmd_abortmp(args):
    s3 = S3Engine(get_s3_connection(),progress_callback=ConsoleProgressCallback())
    num = s3.abortmp(args.url, args.id)
    if num > 0:
        stderr_print(info("%d multiparts aborted." % num))
    else:
        stderr_print(error("No multiparts to abort."))


def cmd_listmp(args):
    s3 = S3Engine(get_s3_connection(),progress_callback=ConsoleProgressCallback())
    mps = s3.listmp(args.url)
    mpn = 0

    if len(mps) == 0:
        stderr_print(error("No multiparts."))
        return

    stderr_print(info("%d multiparts:" % len(mps)))
    for mp in mps:
        print(info("%d:" % mpn) + "\t%s\n\tid='%s'" % (mp[0], mp[1]))
        mpn += 1

if __name__ == "__main__":

    parser = argparse.ArgumentParser(prog=info(sys.argv[0]), description=Fore.GREEN + "S3Engine" + Fore.RESET)

    parser.add_argument("-d", "--debug", action="store_true", help="enable pycharm debugging")

    subparsers = parser.add_subparsers()

    parser_put = subparsers.add_parser("put", help="Upload single file to S3")
    parser_put.add_argument("file", type=str, help="path to filename to upload")
    parser_put.add_argument("url", type=str, help="s3://<bucket>/<object>")
    parser_put.add_argument("-p", "--parallel", action="store_true", help="upload in parallel")
    parser_put.set_defaults(func=cmd_put)

    parser_get = subparsers.add_parser("get", help="Download single file from S3")
    parser_get.add_argument("url", type=str, help="s3://<bucket>/<object>")
    parser_get.add_argument("file", type=str, help="path to filename to download")
    parser_get.add_argument("-p", "--parallel", action="store_true", help="download in parallel")
    parser_get.set_defaults(func=cmd_get)

    parser_cp = subparsers.add_parser("cp", help="Copy one S3Url to another")
    parser_cp.add_argument("url_from", type=str, help="from s3://<bucket>/<object>")
    parser_cp.add_argument("url_to", type=str, help="to s3://<bucket>/<object>")
    parser_cp.add_argument("-p", "--parallel", action="store_true", help="copy in parallel")
    parser_cp.set_defaults(func=cmd_cp)

    parser_rm = subparsers.add_parser("rm", help="Delete keys from S3")
    parser_rm.add_argument("url", type=str, help="from s3://<bucket>/<object>")
    parser_rm.add_argument("-r", "--recursive", action="store_true", help="remove recursively")
    parser_rm.set_defaults(func=cmd_rm)

    parser_ls = subparsers.add_parser("ls", help="List keys in S3")
    parser_ls.add_argument("url", type=str, help="from s3://<bucket>/<object>")
    parser_ls.add_argument("-r", "--recursive", action="store_true", help="list recursively")
    parser_ls.set_defaults(func=cmd_ls)

    parser_abortmp = subparsers.add_parser("abortmp", help="Abort multipart uploads prefixed by S3Url")
    parser_abortmp.add_argument("url", type=str, help="s3://<bucket> or s3://<bucket>/<object>")
    parser_abortmp.add_argument("-i", "--id", type=str, default=None, help="specific multipart id")
    parser_abortmp.set_defaults(func=cmd_abortmp)

    parser_listmp = subparsers.add_parser("listmp", help="List pending multipart uploads prefixed by S3Url")
    parser_listmp.add_argument("url", type=str, help="s3://<bucket> or s3://<bucket>/<object>")
    parser_listmp.set_defaults(func=cmd_listmp)

    args = parser.parse_args()

    if args.debug:
        import pydevd
        pydevd.settrace("127.0.0.1", port=12345, stdoutToServer=True, stderrToServer=True)

    try:
        args.func(args)
    except S3EngineException, e:
        if e.error_code != S3EngineException.ABORTED_BY_USER:
            raise


