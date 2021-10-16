#include"stdafx.h"

CLC7JTR::CLC7JTR(ILC7AccountList *accountlist, ILC7CommandControl *ctrl)
{TR;
	m_ctrl=ctrl;
	m_accountlist=accountlist;
	m_bReset=true;
	m_pJTRWorker=NULL;
	m_ctx=NULL;
	m_is_error=false;
	m_bStopRequested=false;
}

CLC7JTR::~CLC7JTR()
{TR;
	if(m_pJTRWorker)
	{
		if(m_pJTRWorker->isRunning())
		{
			Q_ASSERT(false);
		}
		delete m_pJTRWorker;
	}
	if(m_ctx)
	{
		delete m_ctx;
	}
}

QString CLC7JTR::GetCPUNodeAlgorithm(fourcc hashtype)
{
	TR;

	QString algo;
	
	if (hashtype == FOURCC(HASHTYPE_LM)) algo = "LM";
	else if (hashtype == FOURCC(HASHTYPE_NT))	algo = "NT";
	else if (hashtype == FOURCC(HASHTYPE_LM_CHALRESP)) algo = "netlm";
	else if (hashtype == FOURCC(HASHTYPE_NTLM_CHALRESP)) algo = "netntlm";
	else if (hashtype == FOURCC(HASHTYPE_LM_CHALRESP_V2)) algo = "netlmv2";
	else if (hashtype == FOURCC(HASHTYPE_NTLM_CHALRESP_V2)) algo = "netntlmv2";
	else if (hashtype == FOURCC(HASHTYPE_UNIX_DES)) algo = "descrypt";
	else if (hashtype == FOURCC(HASHTYPE_UNIX_MD5)) algo = "md5crypt";
	else if (hashtype == FOURCC(HASHTYPE_UNIX_BLOWFISH)) algo = "bcrypt";
	else if (hashtype == FOURCC(HASHTYPE_UNIX_SHA256)) algo = "sha256crypt";
	else if (hashtype == FOURCC(HASHTYPE_UNIX_SHA512)) algo = "sha512crypt";
	else if (hashtype == FOURCC(HASHTYPE_AIX_MD5)) algo = "aix-smd5";
	else if (hashtype == FOURCC(HASHTYPE_AIX_SHA1)) algo = "aix-ssha1";
	else if (hashtype == FOURCC(HASHTYPE_AIX_SHA256)) algo = "aix-ssha256";
	else if (hashtype == FOURCC(HASHTYPE_AIX_SHA512)) algo = "aix-ssha512";
	else if (hashtype == FOURCC(HASHTYPE_MAC_SHA1)) algo = "xsha";
	else if (hashtype == FOURCC(HASHTYPE_MAC_SHA512)) algo = "xsha512";
	else if (hashtype == FOURCC(HASHTYPE_MAC_PBKDF2_SHA512)) algo = "PBKDF2-HMAC-SHA512";

	return algo;
}

QString CLC7JTR::GetOpenCLNodeAlgorithm(fourcc hashtype)
{TR;
	QString algo;

	if (hashtype == FOURCC(HASHTYPE_LM)) algo = "LM-opencl";
	else if (hashtype == FOURCC(HASHTYPE_NT))	algo = "nt-opencl";
	else if (hashtype == FOURCC(HASHTYPE_LM_CHALRESP)) algo = "";
	else if (hashtype == FOURCC(HASHTYPE_NTLM_CHALRESP)) algo = "";
	else if (hashtype == FOURCC(HASHTYPE_LM_CHALRESP_V2)) algo = "";
	else if (hashtype == FOURCC(HASHTYPE_NTLM_CHALRESP_V2)) algo = "ntlmv2-opencl";
	else if (hashtype == FOURCC(HASHTYPE_UNIX_DES)) algo = "descrypt-opencl";
	else if (hashtype == FOURCC(HASHTYPE_UNIX_MD5)) algo = "md5crypt-opencl";
	else if (hashtype == FOURCC(HASHTYPE_UNIX_BLOWFISH)) algo = "bcrypt-opencl";
	else if (hashtype == FOURCC(HASHTYPE_UNIX_SHA256)) algo = "sha256crypt-opencl";
	else if (hashtype == FOURCC(HASHTYPE_UNIX_SHA512)) algo = "sha512crypt-opencl";
	else if (hashtype == FOURCC(HASHTYPE_MAC_SHA1)) algo = "xsha";
	else if (hashtype == FOURCC(HASHTYPE_MAC_SHA512)) algo = "xsha512";
	else if (hashtype == FOURCC(HASHTYPE_MAC_PBKDF2_SHA512)) algo = "PBKDF2-HMAC-SHA512-opencl";

	return algo;
}

/*
QString CLC7JTR::GetCUDANodeAlgorithm(fourcc hashtype)
{TR;
	QString algo;
	
	if (hashtype == FOURCC(HASHTYPE_LM)) algo = "";
	else if (hashtype == FOURCC(HASHTYPE_NT))	algo = "";
	else if (hashtype == FOURCC(HASHTYPE_LM_CHALRESP)) algo = "";
	else if (hashtype == FOURCC(HASHTYPE_NTLM_CHALRESP)) algo = "";
	else if (hashtype == FOURCC(HASHTYPE_LM_CHALRESP_V2)) algo = "";
	else if (hashtype == FOURCC(HASHTYPE_NTLM_CHALRESP_V2)) algo = "";
	else if (hashtype == FOURCC(HASHTYPE_UNIX_DES)) algo = "";
	else if (hashtype == FOURCC(HASHTYPE_UNIX_MD5)) algo = "md5crypt-cuda";
	else if (hashtype == FOURCC(HASHTYPE_UNIX_BLOWFISH)) algo = "";
	else if (hashtype == FOURCC(HASHTYPE_UNIX_SHA256)) algo = "sha256crypt-cuda";
	else if (hashtype == FOURCC(HASHTYPE_UNIX_SHA512)) algo = "sha512crypt-cuda";
	else if (hashtype == FOURCC(HASHTYPE_MAC_SHA1)) algo = "";
	else if (hashtype == FOURCC(HASHTYPE_MAC_SHA512)) algo = "xsha512-cuda";
	else if (hashtype == FOURCC(HASHTYPE_MAC_PBKDF2_SHA512)) algo = "";

	return algo;
}
*/

bool CLC7JTR::ConfigurePassNodes(JTRPASS & pass)
{TR;
	ILC7Settings *settings=g_pLinkage->GetSettings();

	// Only one cpu node for single cracks
	if (pass.jtrmode == "single")
	{
		JTRPASSNODE passnode;
		passnode.node = -1;
		passnode.nodecount = -1;
		passnode.gpu_enable = false;
		passnode.gpu_jtrindex = -1;
		passnode.gpu_platform = GPU_NONE;
		passnode.jtrversion = "sse2";
		passnode.node_algorithm = GetCPUNodeAlgorithm(pass.hashtype);
		passnode.preflight_node_algorithm = GetCPUNodeAlgorithm(pass.hashtype);

		pass.nodes.append(passnode);
	}
	else
	{
		if (!m_cal.isValid() )
		{
			// Add nodes for default CPU when no calibration is available
			int cpucount = settings->value(UUID_LC7JTRPLUGIN.toString() + ":cpucount", 1).toInt();
			for (int i = 0; i < cpucount; i++)
			{
				JTRPASSNODE passnode;
				passnode.node = -1;
				passnode.nodecount = -1;
				passnode.gpu_enable = false;
				passnode.gpu_jtrindex = -1;
				passnode.gpu_platform = GPU_NONE;
				passnode.jtrversion = "sse2";
				passnode.node_algorithm = GetCPUNodeAlgorithm(pass.hashtype);
				passnode.preflight_node_algorithm = GetCPUNodeAlgorithm(pass.hashtype);

				pass.nodes.append(passnode);
			}
		}
		else
		{
			GPUPLATFORM gpuplatform;
			QString cpuinstructionset;
			QString jtrkernel;
			QMap<int, QString> extra_opencl_kernel_args;
			if (!m_cal.GetHashTypePreferredProcessor(pass.jtrmode == "mask", pass.hashtype, gpuplatform, cpuinstructionset, jtrkernel, extra_opencl_kernel_args))
			{
				return false;
			}

			if (gpuplatform == GPU_NONE)
			{
				// Add nodes for CPU
				for (int i = 0; i < m_cal.CPUThreadCount(); i++)
				{
					JTRPASSNODE passnode;
					passnode.node = -1;
					passnode.nodecount = -1;
					passnode.gpu_enable = false;
					passnode.gpu_jtrindex = -1;
					passnode.gpu_platform = GPU_NONE;
					passnode.jtrversion = cpuinstructionset;
					passnode.node_algorithm = jtrkernel;
					passnode.preflight_node_algorithm = GetCPUNodeAlgorithm(pass.hashtype);

					pass.nodes.append(passnode);
				}
			}
			else
			{
				// Add nodes for GPU
				QVector<GPUINFO> gi = m_cal.GPUInfo();
				foreach(GPUINFO g, gi)
				{
					if (g.platform != gpuplatform)
					{
						continue;
					}

					QString platform;
					if (g.platform == GPU_OPENCL)
					{
						platform = "OpenCL";
					}
					//else if (g.platform == GPU_CUDA)
					//{
					//	platform = "CUDA";
					//}

					bool enablegpu = settings->value(UUID_LC7JTRPLUGIN.toString() + QString(":enablegpu_%1_%2").arg(platform).arg(g.jtrindex)).toBool();
					if (!enablegpu)
					{
						// Skip this if it's manually disabled
						continue;
					}

					// add the JTRPASSNODE
					JTRPASSNODE passnode;
					passnode.gpu_enable = true;
					passnode.gpu_jtrindex = g.jtrindex;
					passnode.gpu_platform = g.platform;
					passnode.node = -1;
					passnode.nodecount = -1;
					passnode.node_algorithm = jtrkernel;
					passnode.jtrversion = "sse2"; // Any version will do here since they all have opencl and cuda, pick the most compatible
					passnode.preflight_node_algorithm = GetCPUNodeAlgorithm(pass.hashtype);
					passnode.extra_opencl_kernel_args = extra_opencl_kernel_args.value(g.jtrindex);

					pass.nodes.append(passnode);
				}
			}
		}
	}

	// Set nodecount for all nodes
	int node, nodecount=pass.nodes.count();
	for(node=0;node<nodecount;node++)
	{
		pass.nodes[node].node=node;
		pass.nodes[node].nodecount=nodecount;
	}

	return true;
}


QByteArray CLC7JTR::ConvertToEncoding(QString str, QUuid encoding)
{
	if (encoding.isNull())
	{
		encoding = m_ctx->m_current_input_encoding;
	}

	QTextCodec *codec = m_encodingcache[encoding];
	
	QByteArray out;
	if (!codec)
	{
		ILC7PresetManager *manager = g_pLinkage->GetPresetManager();
		ILC7PresetGroup *encodings = manager->presetGroup(QString("%1:encodings").arg(UUID_LC7JTRPLUGIN.toString()));
		ILC7Preset *preset = encodings->presetById(encoding);

		codec = QTextCodec::codecForName(preset->config().toMap()["icuname"].toString().toLatin1());
		if (!codec)
		{
			Q_ASSERT(0);
			return str.toLatin1();
		}
		m_encodingcache[encoding] = codec;
	}

	out = codec->fromUnicode(str);
	return out;
}

bool CLC7JTR::ProcessHashes(quint32 hashtype, bool create_index, bool create_hash_file)
{TR;
	
	if(!m_ctx || m_ctx->m_temporary_dir.isEmpty())
	{
		return false;
	}

	if (create_index)
	{
		m_accts_by_firsthalf.clear();
		m_accts_by_secondhalf.clear();
		m_accts_by_hash.clear();
	}

	QFile hashfile;
	if (create_hash_file)
	{
		QDir tempdir(m_ctx->m_temporary_dir);
		QString hashespath = tempdir.filePath(QString("hashes"));
		QFile::remove(hashespath); // always start fresh
		hashfile.setFileName(hashespath);
		if (!hashfile.open(QIODevice::WriteOnly))
		{
			return false;
		}
	}

	m_accountlist->Acquire();

	int cnt=(int)m_accountlist->GetAccountCount();
	for(int i=0;i<cnt;i++)
	{
		const LC7Account *acct=m_accountlist->GetAccountAtConstPtrFast(i);

		// Split hashes in account into map
		int numhash = -1;
		foreach(LC7Hash lc7hash, acct->hashes)
		{
			numhash++;
			
			QString userinfo = acct->userinfo;
			userinfo.remove(":");

			QByteArray linedata = ConvertToEncoding(QString(":::%1::").arg(userinfo));
			
			if ((hashtype == FOURCC(HASHTYPE_LM) || hashtype==0) && lc7hash.hashtype == FOURCC(HASHTYPE_LM))
			{
				QByteArray hashhex = lc7hash.hash;
				if (hashhex.length() != 32)
				{
					continue;
				}
				QByteArray firsthalf(hashhex.mid(0, 16));
				QByteArray secondhalf(hashhex.mid(16, 16));

				QByteArray firsthalfstr = QByteArray("$LM$") + firsthalf.toLower();
				QByteArray secondhalfstr = QByteArray("$LM$") + secondhalf.toLower();

				if (!(lc7hash.crackstate & CRACKSTATE_FIRSTHALF_CRACKED))
				{
					if (create_hash_file)
					{
						hashfile.write(ConvertToEncoding(acct->username) + QByteArray(":") + firsthalfstr + linedata + QByteArray("\n"));
					}
					if (create_index)
					{
						m_accts_by_firsthalf.insert(firsthalfstr, QPair<int, int>(i, numhash));
					}
				}
				if (!(lc7hash.crackstate & CRACKSTATE_SECONDHALF_CRACKED))
				{
					if (create_hash_file)
					{
						hashfile.write(ConvertToEncoding(acct->username) + QByteArray(":") + secondhalfstr + linedata + QByteArray("\n"));
					}
					if (create_index)
					{
						m_accts_by_secondhalf.insert(secondhalfstr, QPair<int, int>(i, numhash));
					}
				}
			}
			else if ((hashtype == FOURCC(HASHTYPE_NT) || hashtype == 0) && lc7hash.hashtype == FOURCC(HASHTYPE_NT))
			{
				QByteArray hashhex = lc7hash.hash;
				if (hashhex.length() != 32)
				{
					continue;
				}
				QByteArray hashstr = QByteArray("$NT$") + hashhex.toLower();

				if (lc7hash.crackstate != CRACKSTATE_CRACKED)
				{
					if (create_hash_file)
					{
						hashfile.write(ConvertToEncoding(acct->username) + QByteArray(":") + hashstr + linedata + QByteArray("\n"));
					}
					if (create_index)
					{
						m_accts_by_hash.insert(hashstr, QPair<int, int>(i, numhash));
					}
				}
			}
			else if ((hashtype == FOURCC(HASHTYPE_LM_CHALRESP) || hashtype == 0) && lc7hash.hashtype == FOURCC(HASHTYPE_LM_CHALRESP))
			{
				QByteArray hashhex = lc7hash.hash;
				if (hashhex.length() != 16 + 48 + 1)
				{
					continue;
				}
				QByteArray chal(hashhex.mid(0, 16).toLower());
				QByteArray resp(hashhex.mid(17, 48).toLower());

				QByteArray hashstr = QByteArray("$NETLM$") + chal + QByteArray("$") + resp;
				if (lc7hash.crackstate != CRACKSTATE_CRACKED)
				{
					if (create_hash_file)
					{
						hashfile.write(ConvertToEncoding(acct->username) + QByteArray(":") + hashstr + linedata + QByteArray("\n"));
					}
					if (create_index)
					{
						m_accts_by_hash.insert(hashstr, QPair<int, int>(i, numhash));
					}
				}
			}
			else if ((hashtype == FOURCC(HASHTYPE_NTLM_CHALRESP) || hashtype == 0) && lc7hash.hashtype == FOURCC(HASHTYPE_NTLM_CHALRESP))
			{
				QByteArray hashhex = lc7hash.hash;
				if (hashhex.length() != 16 + 48 + 1)
				{
					continue;
				}
				QByteArray chal(hashhex.mid(0, 16).toLower());
				QByteArray resp(hashhex.mid(17, 48).toLower());

				QByteArray hashstr = QByteArray("$NETNTLM$") + chal + QByteArray("$") + resp;

				if (lc7hash.crackstate != CRACKSTATE_CRACKED)
				{
					if (create_hash_file)
					{
						hashfile.write(ConvertToEncoding(acct->username) + QByteArray(":") + hashstr + linedata + QByteArray("\n"));
					}
					if (create_index)
					{
						m_accts_by_hash.insert(hashstr, QPair<int, int>(i, numhash));
					}
				}
			}
			else if ((hashtype == FOURCC(HASHTYPE_LM_CHALRESP_V2) || hashtype == 0) && lc7hash.hashtype == FOURCC(HASHTYPE_LM_CHALRESP_V2))
			{
				QByteArray hashhex = lc7hash.hash;
				if (hashhex.length() != 16 + 1 + 32 + 1 + 16)
				{
					continue;
				}
				QByteArray schal(hashhex.mid(0, 16).toLower());
				QByteArray resp(hashhex.mid(17, 32).toLower());
				QByteArray cchal(hashhex.mid(50).toLower());

				QByteArray hashstr = QByteArray("$NETLMv2$") + ConvertToEncoding(acct->username).toUpper() + ConvertToEncoding(acct->domain).toUpper() + QByteArray("$") + schal + QByteArray("$") + resp + QByteArray("$") + cchal;

				if (lc7hash.crackstate != CRACKSTATE_CRACKED)
				{
					if (create_hash_file)
					{
						hashfile.write(ConvertToEncoding(acct->username) + QByteArray(":") + hashstr + linedata + QByteArray("\n"));
					}
					if (create_index)
					{
						m_accts_by_hash.insert(hashstr, QPair<int, int>(i, numhash));
					}
				}
			}
			else if ((hashtype == FOURCC(HASHTYPE_NTLM_CHALRESP_V2) || hashtype == 0) && lc7hash.hashtype == FOURCC(HASHTYPE_NTLM_CHALRESP_V2))
			{
				QByteArray hashhex = lc7hash.hash;
				if (hashhex.length() <= 16 + 1 + 32 + 1 + 16)
				{
					continue;
				}
				QByteArray schal(hashhex.mid(0, 16).toLower());
				QByteArray resp(hashhex.mid(17, 32).toLower());
				QByteArray cchal(hashhex.mid(50).toLower());

				QByteArray hashstr = QByteArray("$NETNTLMv2$") + ConvertToEncoding(acct->username).toUpper() + ConvertToEncoding(acct->domain).toUpper() + QByteArray("$") + schal + QByteArray("$") + resp + QByteArray("$") + cchal;

				if (lc7hash.crackstate != CRACKSTATE_CRACKED)
				{
					if (create_hash_file)
					{
						hashfile.write(ConvertToEncoding(acct->username) + QByteArray(":") + hashstr + linedata + QByteArray("\n"));
					}
					if (create_index)
					{
						m_accts_by_hash.insert(hashstr, QPair<int, int>(i, numhash));
					}
				}
			}
			else if ((hashtype == FOURCC(HASHTYPE_UNIX_DES) || hashtype == 0) && lc7hash.hashtype == FOURCC(HASHTYPE_UNIX_DES))
			{
				QByteArray hash = lc7hash.hash;
				if (hash.length() != 13)
				{
					continue;
				}
				QByteArray hashstr = hash;

				if (lc7hash.crackstate != CRACKSTATE_CRACKED)
				{
					if (create_hash_file)
					{
						hashfile.write(ConvertToEncoding(acct->username) + QByteArray(":") + hashstr + linedata +  QByteArray("\n"));
					}
					if (create_index)
					{
						m_accts_by_hash.insert(hashstr, QPair<int, int>(i, numhash));
					}
				}
			}
			else if ((hashtype == FOURCC(HASHTYPE_UNIX_MD5) || hashtype == 0) && lc7hash.hashtype == FOURCC(HASHTYPE_UNIX_MD5))
			{
				QByteArray hash = lc7hash.hash;
				if (!(hash.startsWith("$1") || hash.startsWith("$apr1")))
				{
					continue;
				}
				QByteArray hashstr = hash;

				if (lc7hash.crackstate != CRACKSTATE_CRACKED)
				{
					if (create_hash_file)
					{
						hashfile.write(ConvertToEncoding(acct->username) + QByteArray(":") + hashstr + linedata + QByteArray("\n"));
					}
					if (create_index)
					{
						m_accts_by_hash.insert(hashstr, QPair<int, int>(i, numhash));
					}
				}
			}
			else if ((hashtype == FOURCC(HASHTYPE_UNIX_BLOWFISH) || hashtype == 0) && lc7hash.hashtype == FOURCC(HASHTYPE_UNIX_BLOWFISH))
			{
				QByteArray hash = lc7hash.hash;
				if (!(hash.startsWith("$2")))
				{
					continue;
				}
				QByteArray hashstr = hash;

				if (lc7hash.crackstate != CRACKSTATE_CRACKED)
				{
					if (create_hash_file)
					{
						hashfile.write(ConvertToEncoding(acct->username) + QByteArray(":") + hashstr + linedata + QByteArray("\n"));
					}
					if (create_index)
					{
						m_accts_by_hash.insert(hashstr, QPair<int, int>(i, numhash));
					}
				}
			}
			else if ((hashtype == FOURCC(HASHTYPE_UNIX_SHA256) || hashtype == 0) && lc7hash.hashtype == FOURCC(HASHTYPE_UNIX_SHA256))
			{
				QByteArray hash = lc7hash.hash;
				if (!(hash.startsWith("$5")))
				{
					continue;
				}
				QByteArray hashstr = hash;

				if (lc7hash.crackstate != CRACKSTATE_CRACKED)
				{
					if (create_hash_file)
					{
						hashfile.write(ConvertToEncoding(acct->username) + QByteArray(":") + hashstr + linedata + QByteArray("\n"));
					}
					if (create_index)
					{
						m_accts_by_hash.insert(hashstr, QPair<int, int>(i, numhash));
					}
				}
			}
			else if ((hashtype == FOURCC(HASHTYPE_UNIX_SHA512) || hashtype == 0) && lc7hash.hashtype == FOURCC(HASHTYPE_UNIX_SHA512))
			{
				QByteArray hash = lc7hash.hash;
				if (!(hash.startsWith("$6")))
				{
					continue;
				}
				QByteArray hashstr = hash;

				if (lc7hash.crackstate != CRACKSTATE_CRACKED)
				{
					if (create_hash_file)
					{
						hashfile.write(ConvertToEncoding(acct->username) + QByteArray(":") + hashstr + linedata + QByteArray("\n"));
					}
					if (create_index)
					{
						m_accts_by_hash.insert(hashstr, QPair<int, int>(i, numhash));
					}
				}
			}
			else if ((hashtype == FOURCC(HASHTYPE_AIX_MD5) || hashtype == 0) && lc7hash.hashtype == FOURCC(HASHTYPE_AIX_MD5))
			{
				QByteArray hash = lc7hash.hash;
				if (!(hash.startsWith("{smd5}")))
				{
					continue;
				}
				QByteArray hashstr = hash;

				if (lc7hash.crackstate != CRACKSTATE_CRACKED)
				{
					if (create_hash_file)
					{
						hashfile.write(ConvertToEncoding(acct->username) + QByteArray(":") + hashstr + linedata + QByteArray("\n"));
					}
					if (create_index)
					{
						m_accts_by_hash.insert(hashstr, QPair<int, int>(i, numhash));
					}
				}
			}
			else if ((hashtype == FOURCC(HASHTYPE_AIX_SHA1) || hashtype == 0) && lc7hash.hashtype == FOURCC(HASHTYPE_AIX_SHA1))
			{
				QByteArray hash = lc7hash.hash;
				if (!(hash.startsWith("{ssha1}")))
				{
					continue;
				}
				QByteArray hashstr = hash;

				if (lc7hash.crackstate != CRACKSTATE_CRACKED)
				{
					if (create_hash_file)
					{
						hashfile.write(ConvertToEncoding(acct->username) + QByteArray(":") + hashstr + linedata + QByteArray("\n"));
					}
					if (create_index)
					{
						m_accts_by_hash.insert(hashstr, QPair<int, int>(i, numhash));
					}
				}
			}
			else if ((hashtype == FOURCC(HASHTYPE_AIX_SHA256) || hashtype == 0) && lc7hash.hashtype == FOURCC(HASHTYPE_AIX_SHA256))
			{
				QByteArray hash = lc7hash.hash;
				if (!(hash.startsWith("{ssha256}")))
				{
					continue;
				}
				QByteArray hashstr = hash;

				if (lc7hash.crackstate != CRACKSTATE_CRACKED)
				{
					if (create_hash_file)
					{
						hashfile.write(ConvertToEncoding(acct->username) + QByteArray(":") + hashstr + linedata + QByteArray("\n"));
					}
					if (create_index)
					{
						m_accts_by_hash.insert(hashstr, QPair<int, int>(i, numhash));
					}
				}
			}
			else if ((hashtype == FOURCC(HASHTYPE_AIX_SHA512) || hashtype == 0) && lc7hash.hashtype == FOURCC(HASHTYPE_AIX_SHA512))
			{
				QByteArray hash = lc7hash.hash;
				if (!(hash.startsWith("{ssha512}")))
				{
					continue;
				}
				QByteArray hashstr = hash;

				if (lc7hash.crackstate != CRACKSTATE_CRACKED)
				{
					if (create_hash_file)
					{
						hashfile.write(ConvertToEncoding(acct->username) + QByteArray(":") + hashstr + linedata + QByteArray("\n"));
					}
					if (create_index)
					{
						m_accts_by_hash.insert(hashstr, QPair<int, int>(i, numhash));
					}
				}
			}
			else if ((hashtype == FOURCC(HASHTYPE_MAC_SHA1) || hashtype == 0) && lc7hash.hashtype == FOURCC(HASHTYPE_MAC_SHA1))
			{
				QByteArray hash = lc7hash.hash;
				if (hash.length()!=52)
				{
					continue;
				}
				QByteArray hashstr = hash;

				if (lc7hash.crackstate != CRACKSTATE_CRACKED)
				{
					if (create_hash_file)
					{
						hashfile.write(ConvertToEncoding(acct->username) + QByteArray(":") + hashstr + linedata + QByteArray("\n"));
					}
					if (create_index)
					{
						m_accts_by_hash.insert(hashstr, QPair<int, int>(i, numhash));
					}
				}
			}
			else if ((hashtype == FOURCC(HASHTYPE_MAC_SHA512) || hashtype == 0) && lc7hash.hashtype == FOURCC(HASHTYPE_MAC_SHA512))
			{
				QByteArray hash = lc7hash.hash;
				if (!hash.toUpper().startsWith("$LION$"))
				{
					continue;
				}
				QByteArray hashstr = hash;

				if (lc7hash.crackstate != CRACKSTATE_CRACKED)
				{
					if (create_hash_file)
					{
						hashfile.write(ConvertToEncoding(acct->username) + QByteArray(":") + hashstr + linedata + QByteArray("\n"));
					}
					if (create_index)
					{
						m_accts_by_hash.insert(hashstr, QPair<int, int>(i, numhash));
					}
				}
			}
			else if ((hashtype == FOURCC(HASHTYPE_MAC_PBKDF2_SHA512) || hashtype == 0) && lc7hash.hashtype == FOURCC(HASHTYPE_MAC_PBKDF2_SHA512))
			{
				QByteArray hash = lc7hash.hash;
				if (!hash.toLower().startsWith("$pbkdf2-hmac-sha512$") &&
					!hash.toLower().startsWith("$ml$"))
				{
					continue;
				}
				QByteArray hashstr = hash;

				if (lc7hash.crackstate != CRACKSTATE_CRACKED)
				{
					if (create_hash_file)
					{
						hashfile.write(ConvertToEncoding(acct->username) + QByteArray(":") + hashstr + linedata + QByteArray("\n"));
					}
					if (create_index)
					{
						m_accts_by_hash.insert(hashstr, QPair<int, int>(i, numhash));
					}
				}
			}
		}
	}
	
	m_accountlist->Release();

	return true;
}

bool CLC7JTR::AddNTTogglePass(int durationblock)
{TR;
	JTRPASS pass;

	pass.hashtype = FOURCC(HASHTYPE_NT);
	pass.jtrmode = "wordlist";
	pass.passdescription = "NT Case Toggle";
	pass.durationblock = durationblock;

	// Wordlist
	pass.wordlist_file = "$$CRACKED$$";

	// Encoding
	ILC7PresetManager *manager = g_pLinkage->GetPresetManager();
	ILC7PresetGroup *encodings = manager->presetGroup(QString("%1:encodings").arg(UUID_LC7JTRPLUGIN.toString()));
	ILC7Preset *encoding = encodings->presetById(m_ctx->m_current_input_encoding);
	if (encoding == NULL)
	{
		m_error = "Unknown encoding";
		return false;
	}

	QMap<QString, QVariant> encodingconfig = encoding->config().toMap();
	pass.encoding = encodingconfig["jtrname"].toString();

	// Rule
	pass.rule = "NT";
	pass.leet = false;

	// Figure out cpu/gpu node distribution for pass
	if (!ConfigurePassNodes(pass))
	{
		return false;
	}

	pass.passnumber = m_ctx->m_jtrpasses.count();
	m_ctx->m_jtrpasses.append(pass);

	return true;
}


struct WORDCOUNTCACHEVALUE
{
	QDateTime modified;
	quint64 count;
};

static QMap<QString, WORDCOUNTCACHEVALUE> s_wordcountcache;

static bool getWordlistCount(QString wordlistfile, quint64 & count)
{
	QDateTime modified = QFileInfo(wordlistfile).lastModified();
	if (s_wordcountcache.contains(wordlistfile))
	{
		if (s_wordcountcache[wordlistfile].modified == modified)
		{
			count = s_wordcountcache[wordlistfile].count;
			return true;
		}
	}

	TURBOLINECOUNT::CLineCount tlc;
	if (!tlc.open(wordlistfile.toStdString().c_str()))
	{
		return false;
	}
	
	TURBOLINECOUNT::tlc_linecount_t linecount;
	if (!tlc.countLines(linecount))
	{
		return false;
	}
	
	tlc.close();

	count = linecount;

	WORDCOUNTCACHEVALUE wccv;
	wccv.count = count;
	wccv.modified = modified;
	s_wordcountcache[wordlistfile] = wccv;

	return true;
}

bool CLC7JTR::AddPasses(fourcc hashtype, int durationblock)
{TR;
	JTRPASS pass;

	pass.hashtype=hashtype;
	pass.jtrmode=m_config["jtr_mode"].toString();
	pass.durationblock = durationblock;

	QString hashname;
	ILC7PasswordLinkage *plink = GET_ILC7PASSWORDLINKAGE(g_pLinkage);
	LC7HashType lc7hashtype;
	if(!plink->Lookup(hashtype, lc7hashtype, m_error))
	{
		return false;
	}

	if (pass.jtrmode == "single")
	{
		// no configuration necessary
		pass.passdescription = QString("Single Mode Crack (%1)").arg(lc7hashtype.description);
	}
	else if (pass.jtrmode == "wordlist")
	{
		pass.passdescription = QString("Wordlist Mode Crack (%1)").arg(lc7hashtype.description);

		// Wordlist
		pass.wordlist_file = m_config["wordlist"].toString();
		
		// Determine count of wordlist
		m_ctrl->SetStatusText("Counting words in wordlist...");
		m_ctrl->AppendToActivityLog("Counting words in wordlist...\n");
		if (!getWordlistCount(pass.wordlist_file, pass.wordlist_count))
		{
			m_error = "Couldn't count words in dictionary file";
			return false;
		}
		m_ctrl->SetStatusText(QString("%1 words").arg(pass.wordlist_count));
		m_ctrl->AppendToActivityLog(QString("%1 words\n").arg(pass.wordlist_count));

		// Encoding
		QUuid encoding_id = m_config["encoding"].toUuid();
		if (encoding_id.isNull())
		{
			m_error = "Unknown encoding id";
			return false;
		}

		ILC7PresetManager *manager = g_pLinkage->GetPresetManager();
		ILC7PresetGroup *encodings = manager->presetGroup(QString("%1:encodings").arg(UUID_LC7JTRPLUGIN.toString()));
		ILC7Preset *encoding = encodings->presetById(encoding_id);
		if (encoding == NULL)
		{
			m_error = "Unknown encoding";
			return false;
		}

		QMap<QString, QVariant> encodingconfig = encoding->config().toMap();
		pass.encoding = encodingconfig["jtrname"].toString();

		// Rule
		QUuid rule_id = m_config["rule"].toUuid();
		if (rule_id.isNull())
		{
			m_error = "Unknown rule id";
			return false;
		}

		ILC7PresetGroup *rules = manager->presetGroup(QString("%1:rules").arg(UUID_LC7JTRPLUGIN.toString()));
		ILC7Preset *rule = rules->presetById(rule_id);
		if (rule == NULL)
		{
			m_error = "Unknown rule";
			return false;
		}

		QMap<QString, QVariant> ruleconfig = rule->config().toMap();
		pass.rule = ruleconfig["jtrname"].toString();

		pass.leet = m_config["leet"].toBool();

	}
	else if(pass.jtrmode=="incremental")
	{
		pass.passdescription = QString("Incremental Mode Crack (%1)").arg(lc7hashtype.description);

		// figure out which character set we want
		QList<QVariant> csetmap_keys = m_config["csetmap_keys"].toList();
		QList<QVariant> csetmap_values = m_config["csetmap_values"].toList();
		
		QMap<fourcc, QVariant> charsetmap;
		int cnt = csetmap_keys.size();
		for (int i = 0; i < cnt; i++)
		{
			charsetmap[csetmap_keys[i].toUInt()] = csetmap_values[i].toUuid();
		}

		QUuid charset_id;
		if (charsetmap.contains(hashtype))
		{
			charset_id = charsetmap[hashtype].toUuid();
		}
		else
		{
			charset_id = m_config["default_charset"].toUuid();
		}
		if (charset_id.isNull())
		{
			m_error = "Unknown character set id";
			return false;
		}

		ILC7PresetManager *manager = g_pLinkage->GetPresetManager();
		ILC7PresetGroup *charsets = manager->presetGroup(QString("%1:charsets").arg(UUID_LC7JTRPLUGIN.toString()));
		ILC7Preset *charset = charsets->presetById(charset_id);
		if (charset == NULL)
		{
			m_error = "Unknown character set";
			return false;
		}

		QMap<QString, QVariant> charsetconfig = charset->config().toMap();

		QString charsetfile = charsetconfig["file"].toString();
		
		int minchars, maxchars;
		minchars = 0;
		maxchars = 24;

		// Adjust length of crack for hashes that don't go to 24 characters
		if (hashtype == FOURCC(HASHTYPE_LM))
		{
			maxchars = 7;
		}
		else if (hashtype == FOURCC(HASHTYPE_LM_CHALRESP))
		{
			maxchars = 14;
		}
		else if (hashtype == FOURCC(HASHTYPE_LM_CHALRESP_V2))
		{
			maxchars = 14;
		}

		// Trim by character set and requested character range
		if (charsetconfig.contains("num_chars_min") && m_config["num_chars_min"].toInt() > minchars)
		{
			minchars = charsetconfig["num_chars_min"].toInt();
		}
		if (charsetconfig.contains("num_chars_max") && m_config["num_chars_max"].toInt() < maxchars)
		{
			maxchars = charsetconfig["num_chars_max"].toInt();
		}
		if (m_config["enable_min"].toBool() && m_config["num_chars_min"].toInt() > minchars)
		{
			minchars = m_config["num_chars_min"].toInt();
		}
		if (m_config["enable_max"].toBool() && m_config["num_chars_max"].toInt() < maxchars)
		{
			maxchars = m_config["num_chars_max"].toInt();
		}

		pass.num_chars_min=minchars;
		pass.num_chars_max=maxchars;
		
		pass.character_set = charsetfile;

		bool mask_mode = charsetconfig.contains("mask") && charsetconfig.contains("mask_encoding");
		if (mask_mode/* && g_pLinkage->GetSettings()->value(UUID_LC7JTRPLUGIN.toString() + ":enablemask",true).toBool()*/)
		{
			pass.passdescription = QString("Mask Mode Crack (%1)").arg(lc7hashtype.description);

			pass.jtrmode = "mask";
			pass.mask = charsetconfig["mask"].toString();
			
			QUuid mask_encoding = charsetconfig["mask_encoding"].toUuid();

			ILC7PresetManager *manager = g_pLinkage->GetPresetManager();
			ILC7PresetGroup *encodings = manager->presetGroup(QString("%1:encodings").arg(UUID_LC7JTRPLUGIN.toString()));
			ILC7Preset *encoding = encodings->presetById(mask_encoding);
			if (encoding == NULL)
			{
				m_error = "Unknown encoding";
				return false;
			}

			QMap<QString, QVariant> encodingconfig = encoding->config().toMap();
			pass.encoding = encodingconfig["jtrname"].toString();
		}
	}
	
	// Split up mask mode passes
	if (pass.jtrmode == "mask")
	{
		quint32 numchars = qMax(pass.num_chars_min, (quint32)1);
		for (; numchars <= pass.num_chars_max; numchars++)
		{
			JTRPASS maskpass = pass;
			maskpass.num_chars_min = (numchars == 1 && pass.num_chars_min==0)?0:numchars;
			maskpass.num_chars_max = numchars;

			// XXX MASK MODE HACK, REMOVE WHEN MASK MODE CAN HANDLE <=2  2chars
			if (maskpass.num_chars_min <= 2)
			{
				maskpass.jtrmode = "incremental";
			}
			
			// Figure out cpu/gpu node distribution for pass
			if (!ConfigurePassNodes(maskpass))
			{
				return false;
			}

			maskpass.passnumber = m_ctx->m_jtrpasses.count();
			m_ctx->m_jtrpasses.append(maskpass);
		}
	}
	else
	{
		// Figure out cpu/gpu node distribution for pass
		if (!ConfigurePassNodes(pass))
		{
			return false;
		}

		pass.passnumber = m_ctx->m_jtrpasses.count();
		m_ctx->m_jtrpasses.append(pass);
	}

	return true;
}

/*
bool CLC7JTR::PreflightNode(JTRPASS & pass, int node)
{
	if ((pass.jtrmode == "single"))
	{
		// Do nothing for single mode right now
		return true;
	}
	if (pass.jtrmode == "wordlist" && pass.wordlist_file == "$$CRACKED$$")
	{
		// Skip NT toggle pass in calculations since we can't know how many are there
		// but we can add in a theoretical maximum which is a good idea.
		pass.candidates_total = m_accountlist->GetAccountCount();
		return true;
	}

	if (node>0)
	{
		// Only do first node for now, since preflight returns candidate counts for all nodes together
		return true;
	}
	
	CLC7ExecuteJTR::PREFLIGHT preflight;

	xxx preflight hashes?
	QDir tempdir(m_ctx->m_temporary_dir);
	QString hashespath = tempdir.filePath(QString("hashes%1").arg(pass.passnumber));

	CJTRNodeWorker nodeworker(m_ctx, &pass, node, false, QString(), QString(), hashespath);
	if (!nodeworker.preflight(preflight))
	{
		return false;
	}

	quint64 candidates_total = 0;

	if (pass.jtrmode == "wordlist")
	{
		candidates_total = pass.wordlist_count * preflight.wordlist_rule_count;
	}
	else if (pass.jtrmode == "incremental")
	{
		candidates_total = preflight.incremental_candidates;
	}
	else if (pass.jtrmode == "mask")
	{
		candidates_total = preflight.mask_candidates;
	}

	pass.candidates_total += candidates_total;

	return true;
}
*/

bool CLC7JTR::Configure(QMap<QString,QVariant> config)
{TR;
	if(m_ctx)
	{
		return false;
	}

	m_config=config;
	ILC7Settings *settings=g_pLinkage->GetSettings();

	// Get verified calibration
	CLC7Calibration::emptyCalibration(m_cal);
	CLC7Calibration savedcal;

	CLC7Calibration::loadCalibration(savedcal);
	if (savedcal.isValid() && savedcal.configurationMatch(m_cal))
	{
		m_cal = savedcal;
	}

	if (!m_cal.isValid())
	{
		g_pLinkage->GetGUILinkage()->AppendToActivityLog("Warning: No calibration data. Calibrate LC7 to improve performance.");
	}
			
	// Get accounts type
	if(m_accountlist->GetAccountCount()==0)
	{
		m_error="No accounts to crack.";
		return false;
	}

	// Create context if this is a 
	m_ctx = new JTRWORKERCTX();

	if(m_config.contains("stopped_context"))
	{
		QVariant stopctx=m_config.value("stopped_context");
		if (!m_ctx->Load(stopctx))
		{
			m_error = "Saved session is from a newer version of L0phtCrack 7. Update to the latest L0phtCrack 7 build to load it.";
			return false;
		}
	}
	else
	{
		m_ctx->m_duration_unlimited = m_config.value("duration_unlimited",true).toBool();
		m_ctx->m_duration_seconds = (3600 * m_config["duration_hours"].toUInt()) + (60 * m_config["duration_minutes"].toUInt());

		// Figure out what jtr hash types to use and add passes
		
		fourcc fcctype = 0;

		bool lm = false;
		bool nt = false;
		fourcc fcctypelm = 0;
		fourcc fcctypent = 0;

		m_accountlist->Acquire();

		for (int i = 0; i < m_accountlist->GetAccountCount(); i++)
		{
			const LC7Account *acct=m_accountlist->GetAccountAtConstPtrFast(i);
			
			foreach(LC7Hash lc7hash, acct->hashes)
			{
				fourcc fcc = lc7hash.hashtype;

				if (fcc == FOURCC(HASHTYPE_NT))
				{
					nt = true;
					
					// Treat LM and NT as the same thing for this test
					fcctypent = fcc;
					fcc = FOURCC(HASHTYPE_LM);
				}
				else if (fcc == FOURCC(HASHTYPE_LM))
				{
					lm = true;
					fcctypelm = fcc;
				}
				else if (fcc == FOURCC(HASHTYPE_NTLM_CHALRESP))
				{
					nt = true;

					// Treat LMCR and NTCR as the same thing for this test
					fcctypent = fcc;
					fcc = FOURCC(HASHTYPE_LM_CHALRESP);
				}
				else if (fcc == FOURCC(HASHTYPE_LM_CHALRESP))
				{
					lm = true;
					fcctypelm = fcc;
				}
				else if (fcc == FOURCC(HASHTYPE_NTLM_CHALRESP_V2))
				{
					nt = true;
					
					// Treat LMCR2 and NTCR2 as the same thing for this test
					fcctypent = fcc;
					fcc = FOURCC(HASHTYPE_LM_CHALRESP_V2);
				}
				else if (fcc == FOURCC(HASHTYPE_LM_CHALRESP_V2))
				{
					lm = true;
					fcctypelm = fcc;
				}


				if (fcctype == 0)
				{
					fcctype = fcc;
				}
				else
				{
					if (fcctype != fcc)
					{
						m_error = "Don't mix hash types!";
						delete m_ctx;
						m_ctx = NULL;
						
						m_accountlist->Release();

						return false;
					}
				}
			}
		}

		m_accountlist->Release();

		// Get encoding to use everywhere
		if (m_config.contains("encoding"))
		{
			m_ctx->m_current_input_encoding = m_config["encoding"].toUuid();
		}
		else
		{
			m_ctx->m_current_input_encoding = UUID_ENCODING_UTF8;
		}

		bool ok=true;
	
		if(lm && nt)
		{
			// Create three time slots, one for LM, one for Toggle, and one for NT.
			if (!m_ctx->m_duration_unlimited)
			{
				quint32 seconds_total = m_ctx->m_duration_seconds;

				// LM
				m_ctx->m_duration_block_seconds_left.append(seconds_total / 2);
				// Toggle
				// No time slice, just go, it shouldnt take too long to do this
				// NT
				m_ctx->m_duration_block_seconds_left.append(seconds_total - (seconds_total / 2));
			}

			ok = ok && AddPasses(fcctypelm, m_ctx->m_duration_unlimited ? -1 : 0);
			ok = ok && AddNTTogglePass(-1);
			ok = ok && AddPasses(fcctypent, m_ctx->m_duration_unlimited ? -1 : 1);
		}
		else if(lm)
		{
			// Just one time slice
			if (!m_ctx->m_duration_unlimited)
			{
				quint32 seconds_total = m_ctx->m_duration_seconds;

				// LM
				m_ctx->m_duration_block_seconds_left.append(seconds_total);
			}
			
			ok = ok && AddPasses(fcctypelm, m_ctx->m_duration_unlimited ? -1 : 0);
		}
		else if(nt)
		{
			// Just one time slice
			if (!m_ctx->m_duration_unlimited)
			{
				quint32 seconds_total = m_ctx->m_duration_seconds;

				// NT
				m_ctx->m_duration_block_seconds_left.append(seconds_total);
			}
			
			ok = ok && AddPasses(fcctypent, m_ctx->m_duration_unlimited ? -1 : 0);
		}
		else
		{
			// Just one time slice
			if (!m_ctx->m_duration_unlimited)
			{
				quint32 seconds_total = m_ctx->m_duration_seconds;

				// Other
				m_ctx->m_duration_block_seconds_left.append(seconds_total);
			}

			ok = ok && AddPasses(fcctype, m_ctx->m_duration_unlimited ? -1 : 0);
		}

		if(!ok)
		{
			m_error="Failed to add auditing pass";
			delete m_ctx;
			m_ctx=NULL;

			return false;
		}
	}

	// Build hash index
	if (!ProcessHashes(0, true, false))
	{
		return false;
	}

	// Preflight all passes now
	
	/* 
	
	XXX DISABLED FOR NOW 

	for (int pass = 0; pass < m_ctx->m_jtrpasses.size(); pass++)
	{
		m_ctrl->SetStatusText(QString("Preparing (%1/%2)...").arg(pass + 1).arg(m_ctx->m_jtrpasses.size()));

		m_ctx->m_jtrpasses[pass].candidates_done = 0;
		m_ctx->m_jtrpasses[pass].candidates_total = 0;

		/? preflight hashes??
		
		// Preflight all nodes
		for (int node = 0; node < m_ctx->m_jtrpasses[pass].nodes.size(); node++)
		{
			if (!PreflightNode(m_ctx->m_jtrpasses[pass], node))
			{
				return false;
			}
		}
	}
	
	*/

	return true;
}

QMap<QString, QVariant> CLC7JTR::GetConfig()
{TR;
	return m_config;
}

QMap<QString, QVariant> CLC7JTR::GetCheckpointConfig()
{
	TR;
	QMap<QString, QVariant> checkpoint_config = m_config;

	QVariant stopctx;
	m_ctx->Save(stopctx);
	checkpoint_config["stopped_context"] = stopctx;
	
	return checkpoint_config;
}


void CLC7JTR::StartCracking(void)
{TR;
	// Release any previously allocated threads and start fresh
	if(m_pJTRWorker)
	{
		if(m_pJTRWorker->isRunning())
		{
			Q_ASSERT(false);
			return;
		}
		delete m_pJTRWorker;
	}

	// Create thread to start execution
	m_is_error=false;
	m_error="";
	
	m_pJTRWorker = new CJTRWorker(this, m_ctx, m_ctrl, m_accountlist);
	m_pJTRWorker->start();
}

void CLC7JTR::StopCracking(void)
{TR;
	if(!m_pJTRWorker->isRunning())
	{
		Q_ASSERT(false);
		return;
	}
	
	m_bStopRequested=true;

	// Stop cracking
	m_pJTRWorker->stop();
}

void CLC7JTR::ResetCracking(void)
{TR;
	// Ensure we've stopped cracking
	if(m_pJTRWorker->isRunning())
	{
		Q_ASSERT(false);
		return;
	}

	m_config.remove("stopped_context");
}

bool CLC7JTR::CheckCrackingFinished(bool & success)
{
	if(m_pJTRWorker->isFinished())
	{
		QString error;
		if(m_pJTRWorker->lasterror(error))
		{
			m_is_error=true;
			m_error=error;
		}

		success=!m_is_error;
		return true;
	}
	
	return false;
}

void CLC7JTR::ProcessStatus(void)
{TR;
	QMutexLocker lock(&m_statusmutex);

	JTRSTATUS status=m_pJTRWorker->get_status();
//	if(status.length()>0)
//	{
		//m_ctrl->AppendToActivityLog(status+"\n");
	//}

	m_ctrl->SetStatusText(status.status);
	m_ctrl->UpdateCurrentProgressBar((quint32)status.percent_done);

	// xxx figure out how to detect if cracks are partial. (full hash exists for two halves account?)

	QMap<QByteArray,QString> cracked=m_pJTRWorker->get_cracked();
	if(cracked.size()>0)
	{
		m_accountlist->Acquire();

		foreach(QByteArray hash,cracked.keys())
		{
			QString password=cracked[hash];

			QList<QPair<int,int> > firsthalves=m_accts_by_firsthalf.values(hash);
			foreach(auto acctnum,firsthalves)
			{
				LC7Account acct=*(m_accountlist->GetAccountAtConstPtrFast(acctnum.first));
				LC7Hash & lc7hash = acct.hashes[acctnum.second];
				
				if (lc7hash.crackstate == CRACKSTATE_SECONDHALF_CRACKED)
				{
					lc7hash.password = password + lc7hash.password;
					lc7hash.crackstate = CRACKSTATE_CRACKED;
					lc7hash.cracktime = status.secs_total;
					lc7hash.cracktype = m_config["name"].toString();

					m_accountlist->ReplaceAccountAt(acctnum.first, acct);
				}
				else if (lc7hash.crackstate == CRACKSTATE_NOT_CRACKED)
				{
					lc7hash.password = password;
					lc7hash.crackstate = CRACKSTATE_FIRSTHALF_CRACKED;
					lc7hash.cracktime = status.secs_total;
					lc7hash.cracktype = m_config["name"].toString();

					m_accountlist->ReplaceAccountAt(acctnum.first, acct);
				}
				/*
				else
				{
					//Q_ASSERT(0);
					TRDBG("Multiple first halves");
				}
				*/
			}
			QList<QPair<int, int> > secondhalves = m_accts_by_secondhalf.values(hash);
			foreach(auto acctnum,secondhalves)
			{
				LC7Account acct = *(m_accountlist->GetAccountAtConstPtrFast(acctnum.first));
				LC7Hash & lc7hash = acct.hashes[acctnum.second];

				if (lc7hash.crackstate == CRACKSTATE_FIRSTHALF_CRACKED)
				{
					lc7hash.password = lc7hash.password + password;
					lc7hash.crackstate = CRACKSTATE_CRACKED;
					lc7hash.cracktime = status.secs_total;
					lc7hash.cracktype = m_config["name"].toString();

					m_accountlist->ReplaceAccountAt(acctnum.first, acct);
				}
				else if (lc7hash.crackstate == CRACKSTATE_NOT_CRACKED)
				{
					lc7hash.password = password;
					lc7hash.crackstate = CRACKSTATE_SECONDHALF_CRACKED;
					lc7hash.cracktime = status.secs_total;
					lc7hash.cracktype = m_config["name"].toString();

					m_accountlist->ReplaceAccountAt(acctnum.first, acct);
				}
				/*
				else
				{
					//Q_ASSERT(0);
					TRDBG("Multiple second halves");
				}
				*/
			}

			QList<QPair<int,int> > acctnums=m_accts_by_hash.values(hash);
			foreach(auto acctnum, acctnums)
			{
				LC7Account acct=*(m_accountlist->GetAccountAtConstPtrFast(acctnum.first));
				LC7Hash & lc7hash = acct.hashes[acctnum.second];

				if (lc7hash.crackstate == CRACKSTATE_NOT_CRACKED)
				{
					lc7hash.password = password;
					lc7hash.crackstate = CRACKSTATE_CRACKED;
					lc7hash.cracktime = status.secs_total;
					lc7hash.cracktype = m_config["name"].toString();
					
					m_accountlist->ReplaceAccountAt(acctnum.first, acct);
				}
				/*
				else if (acct.crackstate == CRACKSTATE_NOT_CRACKED)
				{
					TRDBG("Hash cracked more than once");
				}
				else
				{
					//Q_ASSERT(0);
					TRDBG("Full hash found as well as part or multiple full hashes found");
				}
				*/
			}
		}
		m_accountlist->Release();
	}
}

void CLC7JTR::Cleanup(void)
{TR;
	// Ensure we've stopped cracking
	if(m_pJTRWorker->isRunning())
	{
		Q_ASSERT(false);
		return;
	}

	// If we stopped with stuff in our temporary dir, save it off
	if(m_bStopRequested)
	{
		QVariant stopctx;
		m_ctx->Save(stopctx);
		m_config["stopped_context"]=stopctx;
	}

	// Delete context
	delete m_ctx;
	m_ctx=NULL;

	m_bStopRequested=false;
	m_is_error=false;
	m_error="";
}
	

bool CLC7JTR::IsReset(void)
{TR;
	return m_bReset;
}

QString CLC7JTR::LastError(void)
{TR;
	return m_error;
}


void CLC7JTR::DisableInstructionSet(QString version)
{
	QString key = UUID_LC7JTRPLUGIN.toString() + QString(":enable%1").arg(version);
	g_pLinkage->GetSettings()->setValue(key, false);
}

QStringList CLC7JTR::GetSupportedInstructionSets(bool include_disabled)
{
	QStringList suppins;

	ILC7CPUInformation *cpuid = g_pLinkage->GetCPUInformation();

	if (cpuid->SSE2())
	{
		QString isetkeysse2 = UUID_LC7JTRPLUGIN.toString() + ":enablesse2";
		if (g_pLinkage->GetSettings()->value(isetkeysse2, true).toBool() || include_disabled)
		{
			suppins.append("sse2");
		}
	}

	if (cpuid->SSSE3())
	{
		QString isetkeyssse3 = UUID_LC7JTRPLUGIN.toString() + ":enablessse3";
		if (g_pLinkage->GetSettings()->value(isetkeyssse3, true).toBool() || include_disabled)
		{
			suppins.append("ssse3");
		}
	}

	if (cpuid->SSE41())
	{
		QString isetkeysse41 = UUID_LC7JTRPLUGIN.toString() + ":enablesse41";
		if (g_pLinkage->GetSettings()->value(isetkeysse41, true).toBool() || include_disabled)
		{
			suppins.append("sse41");
		}
	}
#if (PLATFORM != PLATFORM_WIN32)
	if (cpuid->XOP())
	{
		QString isetkeyxop = UUID_LC7JTRPLUGIN.toString() + ":enablexop";
		if (g_pLinkage->GetSettings()->value(isetkeyxop, true).toBool() || include_disabled)
		{
			suppins.append("xop");
		}
	}

	if (cpuid->AVX() && cpuid->XSAVE() && cpuid->OSXSAVE() && cpuid->XMM_SAVED() && cpuid->YMM_SAVED())
	{
		QString isetkeyavx = UUID_LC7JTRPLUGIN.toString() + ":enableavx";
		if (g_pLinkage->GetSettings()->value(isetkeyavx, true).toBool() || include_disabled)
		{
			suppins.append("avx");
		}
	}
	if (cpuid->AVX() && cpuid->AVX2() && cpuid->XSAVE() && cpuid->OSXSAVE() && cpuid->XMM_SAVED() && cpuid->YMM_SAVED() && cpuid->MOVBE() && cpuid->FMA())
	{
		QString isetkeyavx2 = UUID_LC7JTRPLUGIN.toString() + ":enableavx2";
		if (g_pLinkage->GetSettings()->value(isetkeyavx2, true).toBool() || include_disabled)
		{
			suppins.append("avx2");
		}
	}
#endif
	return suppins;
}

