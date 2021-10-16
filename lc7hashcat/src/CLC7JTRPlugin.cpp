#include<stdafx.h>


CLC7JTRPlugin::CLC7JTRPlugin()
{TR;
	m_pSystemJTR = NULL;
	m_pTechniqueJTR = NULL;
	m_pTechniqueJTRSingleGUI = NULL;
	m_pTechniqueJTRBruteGUI = NULL;
	m_pTechniqueJTRDictionaryGUI = NULL;
	m_pTechniqueCat = NULL;
	m_pBaseCat = NULL;
	m_pSystemCat = NULL;
	m_pSingleAct = NULL;
	m_pBruteAct = NULL;
	m_pDictionaryAct = NULL;
	m_pSettingsAct = NULL;
	m_pCalibrateAct = NULL;

	connect(this, &CLC7JTRPlugin::sig_checkCalibration, this, &CLC7JTRPlugin::slot_checkCalibration, Qt::BlockingQueuedConnection);
}

CLC7JTRPlugin::~CLC7JTRPlugin()
{TR;
}

ILC7Interface *CLC7JTRPlugin::GetInterfaceVersion(QString interface_name)
{
	if (interface_name == "ILC7Plugin")
	{
		return this;
	}
	return NULL;
}
	
QUuid CLC7JTRPlugin::GetID()
{TR;
	return UUID_LC7JTRPLUGIN;
}

QList<QUuid> CLC7JTRPlugin::GetInternalDependencies()
{TR;
	return QList<QUuid>();
}

void CLC7JTRPlugin::CreateAccountTypes(void)
{
	// Register account types we can import
	ILC7PasswordLinkage *passlink = GET_ILC7PASSWORDLINKAGE(g_pLinkage);

#define REGISTER_ACCOUNT_TYPE(TYPE, NAME, DESC) \
	passlink->Register(FOURCC(TYPE), NAME, DESC, "technique", GetID());

	REGISTER_ACCOUNT_TYPE(HASHTYPE_LM, "LM", "Windows LANMAN-Only Hash");
	REGISTER_ACCOUNT_TYPE(HASHTYPE_NT, "NTLM", "Windows NTLM-Only Hash");
	REGISTER_ACCOUNT_TYPE(HASHTYPE_LM_CHALRESP, "LM/CRv1", "Windows LM Challenge/Response v1");
	REGISTER_ACCOUNT_TYPE(HASHTYPE_NTLM_CHALRESP, "NTLM/CRv1", "Windows NTLM Challenge/Response v1");
	REGISTER_ACCOUNT_TYPE(HASHTYPE_LM_CHALRESP_V2, "LM/CRv2", "Windows LM Challenge/Response v2");
	REGISTER_ACCOUNT_TYPE(HASHTYPE_NTLM_CHALRESP_V2, "NTLM/CRv2", "Windows NTLM Challenge/Response v2");
	REGISTER_ACCOUNT_TYPE(HASHTYPE_UNIX_DES, "DES/Crypt", "UNIX DES Crypt");
	REGISTER_ACCOUNT_TYPE(HASHTYPE_UNIX_MD5, "MD5/Crypt", "Linux/BSD/Solaris MD5 Crypt");
	REGISTER_ACCOUNT_TYPE(HASHTYPE_UNIX_BLOWFISH, "BF/Crypt", "Linux/BSD/Solaris Blowfish Crypt");
	REGISTER_ACCOUNT_TYPE(HASHTYPE_UNIX_SHA256, "SHA256/Crypt", "Linux/BSD/Solaris SHA256 Crypt");
	REGISTER_ACCOUNT_TYPE(HASHTYPE_UNIX_SHA512, "SHA512/Crypt", "Linux/BSD/Solaris SHA512 Crypt");
	REGISTER_ACCOUNT_TYPE(HASHTYPE_AIX_MD5, "SMD5/AIX", "AIX Salted MD5");
	REGISTER_ACCOUNT_TYPE(HASHTYPE_AIX_SHA1, "SSHA1/AIX", "AIX Salted SHA-1");
	REGISTER_ACCOUNT_TYPE(HASHTYPE_AIX_SHA256, "SSHA256/AIX", "AIX Salted SHA-256");
	REGISTER_ACCOUNT_TYPE(HASHTYPE_AIX_SHA512, "SSHA512/AIX", "AIX Salted SHA-512");
	REGISTER_ACCOUNT_TYPE(HASHTYPE_MAC_SHA1, "SSHA1/MAC", "Mac OS X 10.4-10.6 Salted SHA-1");
	REGISTER_ACCOUNT_TYPE(HASHTYPE_MAC_SHA512, "SSHA512/MAC", "MAC OS X 10.7 Salted SHA-512");
	REGISTER_ACCOUNT_TYPE(HASHTYPE_MAC_PBKDF2_SHA512, "PBKDF2+SHA512/MAC", "MAC OS X 10.8+ PBKDF2-SALTED-SHA-512");

}


static void defineCharsetPreset(ILC7PresetGroup *charsets, QUuid uuid, QString name, QString desc, QString file, QString mask, QUuid mask_encoding)
{
	ILC7Preset *preset = charsets->presetById(uuid);
	if (preset == NULL)
	{
		preset = charsets->newPreset(-1, uuid);
	}
	
	QDir lc7jtrdir(g_pLinkage->GetPluginsDirectory());
	lc7jtrdir.cd("lc7jtr{9846c8cf-1db1-467e-83c2-c5655aa81936}");
	QString $john_value = lc7jtrdir.absolutePath();

	preset->setName(name);
	preset->setDescription(desc);
	preset->setReadonly(true);

	QMap<QString, QVariant> config;
	config["file"] = QString("%1/%2").arg($john_value).arg(file);
	config["mask"] = mask;
	config["mask_encoding"] = mask_encoding;

	preset->setConfig(config);
}


static void defineBrutePreset(ILC7PresetGroup *brutes, QUuid uuid, QString name, QString desc, int num_chars_min, int num_chars_max, int hours, int minutes, QUuid csetdef, QList<QPair<fourcc, QUuid> > csetmap)
{
	ILC7Preset *preset = brutes->presetById(uuid);
	if (preset == NULL)
	{
		preset = brutes->newPreset(-1, uuid);
	}

	preset->setName(name);
	preset->setDescription(desc);
	preset->setReadonly(true);

	QMap<QString, QVariant> config;
	if (num_chars_min != -1)
	{
		config["enable_min"] = true;
		config["num_chars_min"] = num_chars_min;
	}
	else
	{
		config["enable_min"] = false;
	}
	if (num_chars_max != -1)
	{
		config["enable_max"] = true;
		config["num_chars_max"] = num_chars_max;
	}
	else
	{
		config["enable_max"] = false;
	}
	if (hours == 0 && minutes==0)
	{
		config["duration_unlimited"] = true;
		config["duration_hours"] = 0;
		config["duration_minutes"] = 0;
	}
	else
	{
		config["duration_unlimited"] = false;
		config["duration_hours"] = hours;
		config["duration_minutes"] = minutes;
	}
	
	config["default_charset"] = csetdef;
	QList<QVariant> hashtypes;
	QList<QVariant> csets;
	foreach(auto cset, csetmap)
	{
		hashtypes.append(cset.first);
		csets.append(cset.second);
	}
	config["csetmap_keys"] = hashtypes;
	config["csetmap_values"] = csets;

	preset->setConfig(config);
}

static void defineEncodingPreset(ILC7PresetGroup *encodings, QUuid uuid, QString name, QString desc, QString jtrname, QString languages, QString icuname)
{
	ILC7Preset *preset = encodings->presetById(uuid);
	if (preset == NULL)
	{
		preset = encodings->newPreset(-1, uuid);
	}
	
	preset->setName(name);
	preset->setDescription(desc);
	preset->setReadonly(true);

	QMap<QString, QVariant> config;
	config["languages"] = languages;
	config["jtrname"] = jtrname;
	config["icuname"] = icuname;

	preset->setConfig(config);
}


static void defineRulePreset(ILC7PresetGroup *rules, QUuid uuid, QString name, QString desc, QString jtrname)
{
	ILC7Preset *preset = rules->presetById(uuid);
	if (preset == NULL)
	{
		preset = rules->newPreset(-1, uuid);
	}

	preset->setName(name);
	preset->setDescription(desc);
	preset->setReadonly(true);

	QMap<QString, QVariant> config;
	config["jtrname"] = jtrname;

	preset->setConfig(config);
}



static void defineWordlistPreset(ILC7PresetGroup *wordlist, QUuid uuid, QString name, QString desc, QUuid encoding, QString wordlistfile, QUuid rule, bool leet, int hours, int minutes)
{TR;
	ILC7Preset *preset = wordlist->presetById(uuid);
	if (preset == NULL)
	{
		preset = wordlist->newPreset(-1, uuid);
	}

	preset->setName(name);
	preset->setDescription(desc);
	preset->setReadonly(true);

	QMap<QString, QVariant> config;
	config["encoding"] = encoding;
	config["wordlist"] = wordlistfile;
	config["rule"] = rule;
	config["leet"] = leet;
	if (hours == 0 && minutes == 0)
	{
		config["duration_unlimited"] = true;
		config["duration_hours"] = 0;
		config["duration_minutes"] = 0;
	}
	else
	{
		config["duration_unlimited"] = false;
		config["duration_hours"] = hours;
		config["duration_minutes"] = minutes;
	}

	preset->setConfig(config);
}

void CLC7JTRPlugin::CreatePresetGroups(void)
{TR;
	ILC7PresetManager *manager = g_pLinkage->GetPresetManager();

	// Charset Presets
	ILC7PresetGroup *charsets = manager->presetGroup(QString("%1:charsets").arg(UUID_LC7JTRPLUGIN.toString()));
	if (!charsets)
	{
		charsets = manager->newPresetGroup(QString("%1:charsets").arg(UUID_LC7JTRPLUGIN.toString()));
	}

	defineCharsetPreset(charsets, UUID_CHARSET_UTF8, "UTF-8", "The UTF-8 unicode character set. Contains UTF-8 valid bytes, not code points. 24 character length limit.", "utf8.chr", "?A", UUID_ENCODING_UTF8);
	defineCharsetPreset(charsets, UUID_CHARSET_LATIN1, "Latin-1", "The Latin-1 character set (ISO-8859-1 / CP1252). 24 character length limit.", "latin1.chr", "?A", UUID_ENCODING_ISO_8859_1);
	defineCharsetPreset(charsets, UUID_CHARSET_ASCII, "ASCII", "The ASCII (0-127) character set. 13 character length limit.", "ascii.chr", "?a", UUID_ENCODING_ASCII);
	defineCharsetPreset(charsets, UUID_CHARSET_LM_ASCII, "LM_ASCII", "The subset of the ASCII (0-127) character set used by LANMAN passwords. 7 character length limit.", "lm_ascii.chr", "?a", UUID_ENCODING_ASCII);
	defineCharsetPreset(charsets, UUID_CHARSET_LANMAN, "LANMAN", "The majority of LANMAN characters. 7 character length limit.", "lanman.chr", "?A", UUID_ENCODING_ISO_8859_1);
	defineCharsetPreset(charsets, UUID_CHARSET_ALNUMSPACE, "Alphanumeric+Space", "The character set consisting of uppercase and lowercase letters, numbers, and space. 13 character length limit.", "alnumspace.chr", "[aeionrlstmcdyhubkgpjvfwzxqAEIOLRNSTMCDBYHUPKGJVFWZXQ1023985467 ]", UUID_ENCODING_ISO_8859_1);
	defineCharsetPreset(charsets, UUID_CHARSET_ALNUM, "Alphanumeric", "The character set consisting of uppercase and lowercase letters and numbers. 13 character length limit.", "alnum.chr", "[aeionrlstmcdyhubkgpjvfwzxqAEIOLRNSTMCDBYHUPKGJVFWZXQ1023985467]", UUID_ENCODING_ISO_8859_1);
	defineCharsetPreset(charsets, UUID_CHARSET_ALPHA, "Alphabetic", "The character set consisting of uppercase and lowercase letters. 13 character length limit.", "alpha.chr", "[aeionrlstmcdyhubkgpjvfwzxqAEIOLRNSTMCDBYHUPKGJVFWZXQ]", UUID_ENCODING_ISO_8859_1);
	defineCharsetPreset(charsets, UUID_CHARSET_LOWERNUM, "Lowercase+Numbers", "The character set consisting of lowercase letters and numbers. 13 character length limit.", "lowernum.chr", "[aeionrlstmcdyhubkgpjvfwzxq1023985467 ]", UUID_ENCODING_ISO_8859_1);
	defineCharsetPreset(charsets, UUID_CHARSET_UPPERNUM, "Uppercase+Numbers", "The character set consisting of uppercase letters and numbers. 13 character length limit.", "uppernum.chr", "[AEIOLRNSTMCDBYHUPKGJVFWZXQ1023985467 ]", UUID_ENCODING_ISO_8859_1);
	defineCharsetPreset(charsets, UUID_CHARSET_LOWERSPACE, "Lowercase+Space", "The character set consisting of lowercase letters and space. 13 character length limit.", "lowerspace.chr", "[aeionrlstmcdyhubkgpjvfwzxq ]", UUID_ENCODING_ISO_8859_1);
	defineCharsetPreset(charsets, UUID_CHARSET_LOWER, "Lowercase", "The character set consisting of only lowercase letters. 13 character length limit.", "lower.chr", "?l", UUID_ENCODING_ISO_8859_1);
	defineCharsetPreset(charsets, UUID_CHARSET_UPPER, "Uppercase", "The character set consisting of only uppercase letters. 13 character length limit.", "upper.chr", "?u", UUID_ENCODING_ISO_8859_1);
	defineCharsetPreset(charsets, UUID_CHARSET_DIGITS, "Digits", "The character set consisting of only numbers. 20 character length limit.", "digits.chr", "?d", UUID_ENCODING_ISO_8859_1);

	// Brute Presets

	ILC7PresetGroup *brutes = manager->presetGroup(QString("%1:brute_presets").arg(UUID_LC7JTRPLUGIN.toString()));
	if (!brutes)
	{
		brutes = manager->newPresetGroup(QString("%1:brute_presets").arg(UUID_LC7JTRPLUGIN.toString()));
	}

	QList<QPair<fourcc, QUuid> > csetmap;

	csetmap << QPair<fourcc, QUuid>(FOURCC(HASHTYPE_LM), UUID_CHARSET_UPPERNUM);
	defineBrutePreset(brutes, UUID_BRUTE_FAST, "Fast", "A quick, 1 hour test with an alphanumeric character set.", 0, 7, 1, 0, UUID_CHARSET_ALNUMSPACE, csetmap);
	csetmap.clear();

	csetmap << QPair<fourcc, QUuid>(FOURCC(HASHTYPE_LM), UUID_CHARSET_LM_ASCII);
	defineBrutePreset(brutes, UUID_BRUTE_THOROUGH, "Thorough", "A thorough, 6 hour test, with a large ASCII character set.", 0, 10, 6, 0, UUID_CHARSET_ASCII, csetmap);
	csetmap.clear();

	csetmap << QPair<fourcc, QUuid>(FOURCC(HASHTYPE_LM), UUID_CHARSET_LANMAN);
	defineBrutePreset(brutes, UUID_BRUTE_EXHAUSTIVE, "Exhaustive", "An exhaustive 24 hour test, with the ISO-8859-1/Latin1 character set.", 0, 14, 24, 0, UUID_CHARSET_LATIN1,csetmap);
	csetmap.clear();

	// Encoding presets
	ILC7PresetGroup *encodings = manager->presetGroup(QString("%1:encodings").arg(UUID_LC7JTRPLUGIN.toString()));
	if (!encodings)
	{
		encodings = manager->newPresetGroup(QString("%1:encodings").arg(UUID_LC7JTRPLUGIN.toString()));
	}

	defineEncodingPreset(encodings, UUID_ENCODING_ASCII, "ASCII", "ASCII(US 7 - bit)", "ascii", "US English, UK English", "US-ASCII");
	defineEncodingPreset(encodings, UUID_ENCODING_UTF8, "UTF8", "Unicode(UTF8)", "utf8", "ALL LANGUAGES", "UTF-8");
	defineEncodingPreset(encodings, UUID_ENCODING_ISO_8859_1, "ISO-8859-1", "ISO-8859-1 (Latin1 / ANSI)", "iso-8859-1", "Afrikaans, Albanian, Basque, Breton, Catalan, Corsican, Danish, US English, UK English, Faroes, Galician, German, Icelandic, Indonesian, Irish, Italian, Latin, Leonese, Luxembourgish, Malay, Manx, Norwegian, Occitan, Portuguese, Rhaeto - Romanic, Scottish Gaelic, Spanish, Swahili, Swedish, Walloon","ISO-8859-1");
	defineEncodingPreset(encodings, UUID_ENCODING_ISO_8859_2, "ISO-8859-2", "ISO-8859-2 (Latin2 / ANSI)", "iso-8859-2", "US English, UK English, Bosnian, Croatian, Czech, German, Hungarian, Polish, Serbian Latin, Slovak, Slovene, Upper Sorbian, Lower Sorbian, Turkmen", "ISO-8859-2");
	defineEncodingPreset(encodings, UUID_ENCODING_ISO_8859_7, "ISO-8859-7", "ISO-8859-7 (Latin / Greek)", "iso-8859-7", "US English, UK English, Greek", "ISO-8859-7");
	defineEncodingPreset(encodings, UUID_ENCODING_ISO_8859_15, "ISO-8859-15", "ISO-8859-15 (Latin9)", "iso-8859-15", "Afrikaans, Albanian, Breton, Catalan, Danish, Dutch, US English, UK English, Estonian, Faroese, Finnish, French, Galician, German, Icelandic, Irish, Italian, Kurdish, Latin, Luxembourgish, Malay, Norwegian, Occitan, Portuguese, Rhaeto - Romanic, Scottish Gaelic, Scots, Spanish, Swahili, Swedish, Tagalog, Walloon", "ISO-8859-15");
	defineEncodingPreset(encodings, UUID_ENCODING_KOI8_R, "KOI8-R", "KOI8-R (Russian)", "koi8r", "US English, UK English, Russian, Bulgarian", "KOI8-R");
	defineEncodingPreset(encodings, UUID_ENCODING_CP437, "CP437", "CP437(OEM, PC Extended ASCII)", "cp437", "US English, UK English","cp437");
	defineEncodingPreset(encodings, UUID_ENCODING_CP737, "CP737", "CP737(MS-DOS Greek)", "cp737", "US English, UK English, Greek", "iso-8859-7");
	defineEncodingPreset(encodings, UUID_ENCODING_CP850, "CP850", "CP850(MS-DOS Latin1)", "cp850", "Danish, Dutch, US English, UK English, French, German, Icelandic, Italian, Norwegian, Portuguese, Spanish, Swedish", "cp850");
	defineEncodingPreset(encodings, UUID_ENCODING_CP852, "CP852", "CP852(MS-DOS Latin2)", "cp852", "Bosnian, Croatian, Czech, US English, UK English, Hungarian, Polish, Romanian, Slovak","cp852");
	defineEncodingPreset(encodings, UUID_ENCODING_CP858, "CP858", "CP858(MS-DOS Latin2+Euro)", "cp858", "Danish, Dutch, US English, UK English, French, German, Icelandic, Italian, Norwegian, Portuguese, Spanish, Swedish","cp858");
	defineEncodingPreset(encodings, UUID_ENCODING_CP866, "CP866", "CP866(Cyrillic)", "cp866", "Belarusian, Bosnian, Bulgarian, US English, UK English, Macedonian, Russian, Serbian, Ukrainian, Kazakh, Kyrgyz, Moldovan, Mongolian, Tajik, Uzbek","cp866");
	defineEncodingPreset(encodings, UUID_ENCODING_CP1250, "CP1250", "CP1250(Windows-1250)", "cp1250", "Albanian, Bosnian, Croatian, Czech, US English, UK English, German, Hungarian, Polish, Slovak, Slovene, Serbian, Romanian","cp1250");
	defineEncodingPreset(encodings, UUID_ENCODING_CP1251, "CP1251", "CP1251(Windows-1251)", "cp1251", "Azeri, Belarusian, Bulgarian, Macedonian, Kazakh, Kyrgyz, Mongolian, Russian, Serbian, Tatar, Ukrainian, Uzbek", "cp1251");
	defineEncodingPreset(encodings, UUID_ENCODING_CP1252, "CP1252", "CP1252(Latin1 / Windows-1252)", "cp1252", "Afrikaans, Basque, Catalan, Danish, Dutch, US English, UK English, Faroese, Finnish, French, Galician, German, Icelandic, Indonesian, Italian, Malay, Norwegian, Portuguese, Spanish, Swahili, Swedish","cp1252");
	defineEncodingPreset(encodings, UUID_ENCODING_CP1253, "CP1253", "CP1253(Windows-1253)", "cp1253", "US English, UK English, Greek","cp1253");

	// Rule presets
	ILC7PresetGroup *rules = manager->presetGroup(QString("%1:rules").arg(UUID_LC7JTRPLUGIN.toString()));
	if (!rules)
	{
		rules = manager->newPresetGroup(QString("%1:rules").arg(UUID_LC7JTRPLUGIN.toString()));
	}

	defineRulePreset(rules, UUID_RULE_WORDLIST, "Common", "Common permutations", "Wordlist");
	defineRulePreset(rules, UUID_RULE_BEST64, "Best64", "Best 64 permutations", "best64");
	defineRulePreset(rules, UUID_RULE_WORDLIST_PLUS, "Wordlist Plus", "Common permutations + Best 64 permutationss", "WordlistPlus");
	defineRulePreset(rules, UUID_RULE_JUMBO, "Jumbo", "Jumbo permutations (Single+Common+NT+Extra)", "Jumbo");
	defineRulePreset(rules, UUID_RULE_JUMBO_PLUS, "Jumbo Plus", "Jumbo permutations (Single+Common+NT+Extra+Best64)", "JumboPlus");
	defineRulePreset(rules, UUID_RULE_EXHAUSTIVE, "Exhaustive", "Exhaustive permutations", "KoreLogic");
	defineRulePreset(rules, UUID_RULE_ALL, "All", "All available permutations", "All");
	defineRulePreset(rules, UUID_RULE_SINGLE_EXTRA, "Single-Extra", "Single-mode plus extra permutations", "Single-Extra");
	defineRulePreset(rules, UUID_RULE_EXTRA, "Extra", "Extra rules, including case toggling for up to 7 letters.", "Extra");
	defineRulePreset(rules, UUID_RULE_NT, "NT", "NT-style 14 character case toggler.", "NT");
	defineRulePreset(rules, UUID_RULE_NONE, "None", "No permutations. Just try the words directly.", "None");

	// Wordlist presets
	ILC7PresetGroup *wordlist = manager->presetGroup(QString("%1:dictionary_presets").arg(UUID_LC7JTRPLUGIN.toString()));
	if (!wordlist)
	{
		wordlist = manager->newPresetGroup(QString("%1:dictionary_presets").arg(UUID_LC7JTRPLUGIN.toString()));
	}

	QDir wordlists(g_pLinkage->GetStartupDirectory());
	wordlists.cd("wordlists");
	QString wbig = wordlists.absoluteFilePath("wordlist-big.txt");
	QString whuge = wordlists.absoluteFilePath("wordlist-huge.txt");
	QString wmedium = wordlists.absoluteFilePath("wordlist-medium.txt");
	QString wsmall = wordlists.absoluteFilePath("wordlist-small.txt");

	defineWordlistPreset(wordlist, UUID_DICTIONARY_FAST, "Fast", "A quick wordlist check with many permutations, using a 250000 word ISO-8859-1 dictionary.", UUID_ENCODING_ISO_8859_1, wmedium, UUID_RULE_JUMBO_PLUS, false, 1, 0);
	defineWordlistPreset(wordlist, UUID_DICTIONARY_THOROUGH, "Thorough", "A thorough wordlist check with common letter substitutions, using a 250000 word ISO-8859-1 dictionary.", UUID_ENCODING_ISO_8859_1, wmedium, UUID_RULE_WORDLIST_PLUS, true, 6, 0);
	defineWordlistPreset(wordlist, UUID_DICTIONARY_COMPLEX, "Complex", "A complex wordlist check with many permutations and letter substitutions, using a 78000 word ISO-8859-1 dictionary.", UUID_ENCODING_ISO_8859_1, wsmall, UUID_RULE_JUMBO, true, 24, 0);
	defineWordlistPreset(wordlist, UUID_DICTIONARY_EXHAUSTIVE, "Exhaustive", "An exhaustive wordlist check with extreme permutations, using a 250000 word ISO-8859-1 dictionary.", UUID_ENCODING_ISO_8859_1, wmedium, UUID_RULE_ALL, false, 0, 0);
}


bool CLC7JTRPlugin::Activate()
{	
	CreateAccountTypes();
	CreatePresetGroups();

	m_pSystemJTR=new CSystemJTR();
	m_pTechniqueJTR= new CTechniqueJTR();
	m_pTechniqueJTRSingleGUI = new CTechniqueJTRSingleGUI();
	m_pTechniqueJTRBruteGUI = new CTechniqueJTRBruteGUI();
	m_pTechniqueJTRDictionaryGUI = new CTechniqueJTRDictionaryGUI();
	
	bool bSuccess=true;
	bSuccess &= g_pLinkage->AddComponent(m_pSystemJTR);
	bSuccess &= g_pLinkage->AddComponent(m_pTechniqueJTR);
	bSuccess &= g_pLinkage->AddComponent(m_pTechniqueJTRSingleGUI);
	bSuccess &= g_pLinkage->AddComponent(m_pTechniqueJTRBruteGUI);
	bSuccess &= g_pLinkage->AddComponent(m_pTechniqueJTRDictionaryGUI);

	if (!bSuccess)
	{
		Deactivate();
		return false;
	}

	m_pTechniqueCat=g_pLinkage->CreateActionCategory("technique","Technique","Password auditing techniques");
	m_pBaseCat=m_pTechniqueCat->CreateActionCategory("base","Base","LC7 core auditing system (JtR)");
	m_pSystemCat=g_pLinkage->CreateActionCategory("system","System","Global system options");

	m_pSingleAct = m_pBaseCat->CreateAction(m_pTechniqueJTRSingleGUI->GetID(), "gui", QStringList(),
		"User Info",
		"Use permutations on usernames to determine passwords. Fast for simple passwords, use first.");

	m_pBruteAct = m_pBaseCat->CreateAction(m_pTechniqueJTRBruteGUI->GetID(), "gui", QStringList(),
		"Brute Force",
		"Use exhaustive incremental cracking to brute-force account passwords. Good for complex passwords.");

	m_pDictionaryAct = m_pBaseCat->CreateAction(m_pTechniqueJTRDictionaryGUI->GetID(), "gui", QStringList(),
		"Dictionary",
		"Use word lists or variations on words in a wordlist to recover account passwords. Good for common passwords.");

	m_pSettingsAct = m_pSystemCat->CreateAction(m_pSystemJTR->GetID(), "get_options", QStringList(),
		"JtR Cracking Engine",
		"Options for the John The Ripper cracking engine");

	m_pCalibrateAct = m_pSystemCat->CreateAction(m_pSystemJTR->GetID(), "calibrate", QStringList(),
		"Perform Calibration",
		"Calibrate JtR Cracking Engine");

	g_pLinkage->GetGUILinkage()->AddTopMenuItem(m_pCalibrateAct);

	g_pLinkage->RegisterNotifySessionActivity(QUuid(), this, (void (QObject::*)(ILC7Linkage::SESSION_ACTIVITY, ILC7SessionHandler *))&CLC7JTRPlugin::NotifySessionActivity);

	return true;
}

void CLC7JTRPlugin::NotifySessionActivity(ILC7Linkage::SESSION_ACTIVITY activity, ILC7SessionHandler *handler)
{
	TR;
	switch (activity)
	{
	case ILC7Linkage::SESSION_OPEN_POST:
	case ILC7Linkage::SESSION_NEW_POST:
		CheckCalibration();
		break;
	case ILC7Linkage::SESSION_CLOSE_PRE:
		break;
	}
}


void CLC7JTRPlugin::CheckCalibration(void)
{TR;
	const bool isGuiThread = (QThread::currentThread() == QCoreApplication::instance()->thread());
	if (isGuiThread)
	{
		slot_checkCalibration();
	}
	else
	{
		emit sig_checkCalibration();
	}
}

void CLC7JTRPlugin::slot_checkCalibration(void)
{
	TR;
	// Ensure we have valid calibration data
	CLC7Calibration cal, savedcal;

	CLC7Calibration::emptyCalibration(cal);
	bool had_saved = false;
	CLC7Calibration::loadCalibration(savedcal);
	if (savedcal.isValid() && savedcal.configurationMatch(cal))
	{
		cal = savedcal;
		had_saved = true;
	}

	if (!cal.isValid())
	{
		if (g_pLinkage->GetGUILinkage()->YesNoBoxWithNeverAgain("Perform Calibration?",
			"L0phtCrack 7 can perform faster if you run calibration on your system.\n\n"
			"This is a one-time operation that measures the speed of all cracking methods for each hash type. This is only performed once, and will take a few minutes.\n\n"
			"If you do not calibrate, the default CPU-based algorithms will be chosen.\n\n"
			"Do you wish to run calibration now?",
			QString("%1:perform_calibration").arg(UUID_LC7JTRPLUGIN.toString())))
		{
			CLC7Calibrate cal(g_pLinkage->GetGUILinkage()->GetMainWindow());
			cal.slot_calibrateButton_clicked(false);
			g_pLinkage->GetGUILinkage()->ShadeUI(true);
			cal.exec();
			g_pLinkage->GetGUILinkage()->ShadeUI(false);
		}
	}
}

bool CLC7JTRPlugin::Deactivate()
{TR;
	g_pLinkage->UnregisterNotifySessionActivity(QUuid(), this, (void (QObject::*)(ILC7Linkage::SESSION_ACTIVITY, ILC7SessionHandler *))&CLC7JTRPlugin::NotifySessionActivity);

	ILC7PasswordLinkage *passlink = GET_ILC7PASSWORDLINKAGE(g_pLinkage);
	if (passlink)
	{
		passlink->Unregister(FOURCC(HASHTYPE_LM), "technique", GetID());
		passlink->Unregister(FOURCC(HASHTYPE_NT), "technique", GetID());
		passlink->Unregister(FOURCC(HASHTYPE_LM_CHALRESP), "technique", GetID());
		passlink->Unregister(FOURCC(HASHTYPE_NTLM_CHALRESP), "technique", GetID());
		passlink->Unregister(FOURCC(HASHTYPE_LM_CHALRESP_V2), "technique", GetID());
		passlink->Unregister(FOURCC(HASHTYPE_NTLM_CHALRESP_V2), "technique", GetID());
		passlink->Unregister(FOURCC(HASHTYPE_UNIX_DES), "technique", GetID());
		passlink->Unregister(FOURCC(HASHTYPE_UNIX_MD5), "technique", GetID());
		passlink->Unregister(FOURCC(HASHTYPE_UNIX_BLOWFISH), "technique", GetID());
		passlink->Unregister(FOURCC(HASHTYPE_UNIX_SHA256), "technique", GetID());
		passlink->Unregister(FOURCC(HASHTYPE_UNIX_SHA512), "technique", GetID());
		passlink->Unregister(FOURCC(HASHTYPE_AIX_MD5), "technique", GetID());
		passlink->Unregister(FOURCC(HASHTYPE_AIX_SHA1), "technique", GetID());
		passlink->Unregister(FOURCC(HASHTYPE_AIX_SHA256), "technique", GetID());
		passlink->Unregister(FOURCC(HASHTYPE_AIX_SHA512), "technique", GetID());
		passlink->Unregister(FOURCC(HASHTYPE_MAC_SHA1), "technique", GetID());
		passlink->Unregister(FOURCC(HASHTYPE_MAC_SHA512), "technique", GetID());
		passlink->Unregister(FOURCC(HASHTYPE_MAC_PBKDF2_SHA512), "technique", GetID());
	}

	if (m_pCalibrateAct)
	{
		g_pLinkage->GetGUILinkage()->RemoveTopMenuItem(m_pCalibrateAct);
		
		m_pSystemCat->RemoveAction(m_pCalibrateAct);
		m_pCalibrateAct = NULL;
	}

	if (m_pSettingsAct)
	{
		m_pSystemCat->RemoveAction(m_pSettingsAct);
		m_pSettingsAct = NULL;
	}
	if (m_pSystemCat)
	{
		g_pLinkage->RemoveActionCategory(m_pSystemCat);
		m_pSystemCat = NULL;
	}
	if (m_pSingleAct)
	{
		m_pBaseCat->RemoveAction(m_pSingleAct);
		m_pSingleAct = NULL;
	}
	if (m_pBruteAct)
	{
		m_pBaseCat->RemoveAction(m_pBruteAct);
		m_pBruteAct = NULL;
	}
	if (m_pDictionaryAct)
	{
		m_pBaseCat->RemoveAction(m_pDictionaryAct);
		m_pDictionaryAct = NULL;
	}
	if (m_pBaseCat)
	{
		m_pTechniqueCat->RemoveActionCategory(m_pBaseCat);
		m_pBaseCat = NULL;
	}
	if (m_pTechniqueCat)
	{
		g_pLinkage->RemoveActionCategory(m_pTechniqueCat);
		m_pTechniqueCat = NULL;
	}
	if (m_pSystemJTR)
	{
		g_pLinkage->RemoveComponent(m_pSystemJTR);
		delete m_pSystemJTR;
		m_pSystemJTR = NULL;
	}
	if (m_pTechniqueJTR)
	{
		g_pLinkage->RemoveComponent(m_pTechniqueJTR);
		delete m_pTechniqueJTR;
		m_pTechniqueJTR = NULL;
	}
	if (m_pTechniqueJTRSingleGUI)
	{
		g_pLinkage->RemoveComponent(m_pTechniqueJTRSingleGUI);
		delete m_pTechniqueJTRSingleGUI;
		m_pTechniqueJTRSingleGUI = NULL;
	}
	if (m_pTechniqueJTRBruteGUI)
	{
		g_pLinkage->RemoveComponent(m_pTechniqueJTRBruteGUI);
		delete m_pTechniqueJTRBruteGUI;
		m_pTechniqueJTRBruteGUI = NULL;
	}
	if (m_pTechniqueJTRDictionaryGUI)
	{
		g_pLinkage->RemoveComponent(m_pTechniqueJTRDictionaryGUI);
		delete m_pTechniqueJTRDictionaryGUI;
		m_pTechniqueJTRDictionaryGUI = NULL;
	}

	return true;
}
