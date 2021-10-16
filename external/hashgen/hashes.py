from passlib.hash import sha256_crypt, sha512_crypt
import random
import string
import sys

username=''.join(random.choice(string.ascii_uppercase + string.digits) for _ in range(8))

if sys.argv[1]=="sha256":
	print username+":"+sha256_crypt.encrypt(sys.argv[2],rounds=10000)
elif sys.argv[1]=="sha512":
	print username+":"+sha512_crypt.encrypt(sys.argv[2],rounds=10000)
