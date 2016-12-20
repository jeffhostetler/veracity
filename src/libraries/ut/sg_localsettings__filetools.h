// Difftool: Araxis Merge (araxis)
#define DIFFTOOL__ARAXIS "araxis"
const char* DIFFTOOL__ARAXIS__PATHS__WINDOWS[] =
{
	"C:\\Program Files\\Araxis\\Araxis Merge\\ConsoleCompare.exe",
	"C:\\Program Files (x86)\\Araxis\\Araxis Merge\\ConsoleCompare.exe",
	"ConsoleCompare.exe",
	NULL
};
const char* DIFFTOOL__ARAXIS__PATHS__MAC[] =
{
	"/Applications/Utilities/compare",
	"/Developer/Applications/Utilities/compare",
	"/Applications/compare",
	"/Developer/Applications/compare",
	"/usr/local/bin/compare",
	"/usr/bin/compare",
	"compare",
	NULL
};
const char* DIFFTOOL__ARAXIS__ARGS__WINDOWS[] =
{
	"/2",
	"@TO_WRITABLE||/readonly@",
	"/title1:@FROM_LABEL@",
	"/title2:@TO_LABEL@",
	"@FROM@",
	"@TO@",
	NULL
};
const char* DIFFTOOL__ARAXIS__ARGS__MAC[] =
{
	"-2",
	"-wait",
	"@TO_WRITABLE||-readonly@",
	"-title1:@FROM_LABEL@",
	"-title2:@TO_LABEL@",
	"@FROM@",
	"@TO@",
	NULL
};
const char* DIFFTOOL__ARAXIS__EXITS__MAC[] =
{
	"1", // files are different
	NULL
};
const char* DIFFTOOL__ARAXIS__RESULTS__MAC[] =
{
	SG_DIFFTOOL__RESULT__DIFFERENT__SZ,
	NULL
};

// Mergetool: Araxis Merge (araxis)
#define MERGETOOL__ARAXIS DIFFTOOL__ARAXIS
#define MERGETOOL__ARAXIS__PATHS__WINDOWS DIFFTOOL__ARAXIS__PATHS__WINDOWS
#define MERGETOOL__ARAXIS__PATHS__MAC DIFFTOOL__ARAXIS__PATHS__MAC
const char* MERGETOOL__ARAXIS__ARGS__WINDOWS[] =
{
	"/3",
	"/merge",
	"/a2",
	"/title1:@BASELINE_LABEL@",
	"/title2:@RESULT_LABEL@",
	"/title3:@OTHER_LABEL@",
	"@BASELINE@",
	"@ANCESTOR@",
	"@OTHER@",
	"@RESULT@",
	NULL
};
const char* MERGETOOL__ARAXIS__ARGS__MAC[] =
{
	"-3",
	"-merge",
	"-wait",
	"-a2",
	"-title1:@BASELINE_LABEL@",
	"-title2:@RESULT_LABEL@",
	"-title3:@OTHER_LABEL@",
	"@BASELINE@",
	"@ANCESTOR@",
	"@OTHER@",
	"@RESULT@",
	NULL
};

// Difftool: Beyond Compare (bcompare)
#define DIFFTOOL__BCOMPARE "bcompare"
const char* DIFFTOOL__BCOMPARE__PATHS__WINDOWS[] =
{
	"C:\\Program Files\\Beyond Compare 3\\BComp.exe",
	"C:\\Program Files (x86)\\Beyond Compare 3\\BComp.exe",
	"BComp.exe",
	NULL
};
const char* DIFFTOOL__BCOMPARE__PATHS__LINUX[] =
{
	"/usr/local/bin/bcompare",
	"/usr/bin/bcompare",
	"bcompare",
	NULL
};
const char* DIFFTOOL__BCOMPARE__ARGS__WINDOWS[] =
{
	"/solo",
	"/leftreadonly",
	"@TO_WRITABLE||/rightreadonly@",
	"/lefttitle=@FROM_LABEL@",
	"/righttitle=@TO_LABEL@",
	"@FROM@",
	"@TO@",
	NULL
};
const char* DIFFTOOL__BCOMPARE__ARGS__LINUX[] =
{
	"-solo",
	"-leftreadonly",
	"@TO_WRITABLE||-rightreadonly@",
	"-lefttitle=@FROM_LABEL@",
	"-righttitle=@TO_LABEL@",
	"@FROM@",
	"@TO@",
	NULL
};
const char* DIFFTOOL__BCOMPARE__EXITS[] =
{
	"1",		// binary same
	"2",		// rules-based same
	"11",		// binary differences
	"12",		// similar
	"13",		// rules-based differences
	NULL
};
#define DIFFTOOL__BCOMPARE__EXITS__WINDOWS DIFFTOOL__BCOMPARE__EXITS
#define DIFFTOOL__BCOMPARE__EXITS__LINUX DIFFTOOL__BCOMPARE__EXITS
const char* DIFFTOOL__BCOMPARE__RESULTS[] =
{
	SG_DIFFTOOL__RESULT__SAME__SZ,
	SG_DIFFTOOL__RESULT__SAME__SZ,
	SG_DIFFTOOL__RESULT__DIFFERENT__SZ,
	SG_DIFFTOOL__RESULT__DIFFERENT__SZ,
	SG_DIFFTOOL__RESULT__DIFFERENT__SZ,
	NULL
};
#define DIFFTOOL__BCOMPARE__RESULTS__WINDOWS DIFFTOOL__BCOMPARE__RESULTS
#define DIFFTOOL__BCOMPARE__RESULTS__LINUX DIFFTOOL__BCOMPARE__RESULTS

// Mergetool: Beyond Compare (bcompare)
#define MERGETOOL__BCOMPARE DIFFTOOL__BCOMPARE
#define MERGETOOL__BCOMPARE__PATHS__WINDOWS DIFFTOOL__BCOMPARE__PATHS__WINDOWS
#define MERGETOOL__BCOMPARE__PATHS__LINUX DIFFTOOL__BCOMPARE__PATHS__LINUX
const char* MERGETOOL__BCOMPARE__ARGS__WINDOWS[] =
{
	"/solo",
	"/readonly",
	"/lefttitle=@BASELINE_LABEL@",
	"/righttitle=@OTHER_LABEL@",
	"/centertitle=@ANCESTOR_LABEL@",
	"/outputtitle=@RESULT_LABEL@",
	"@BASELINE@",
	"@OTHER@",
	"@ANCESTOR@",
	"@RESULT@",
	NULL
};
const char* MERGETOOL__BCOMPARE__ARGS__LINUX[] =
{
	"-solo",
	"-readonly",
	"-lefttitle=@BASELINE_LABEL@",
	"-righttitle=@OTHER_LABEL@",
	"-centertitle=@ANCESTOR_LABEL@",
	"-outputtitle=@RESULT_LABEL@",
	"@BASELINE@",
	"@OTHER@",
	"@ANCESTOR@",
	"@RESULT@",
	NULL
};
const char* MERGETOOL__BCOMPARE__EXITS[] =
{
	"101", // user didn't save anything
	NULL
};
#define MERGETOOL__BCOMPARE__EXITS__WINDOWS MERGETOOL__BCOMPARE__EXITS
#define MERGETOOL__BCOMPARE__EXITS__LINUX MERGETOOL__BCOMPARE__EXITS
const char* MERGETOOL__BCOMPARE__RESULTS[] =
{
	SG_MERGETOOL__RESULT__CANCEL__SZ,
	NULL
};
#define MERGETOOL__BCOMPARE__RESULTS__WINDOWS MERGETOOL__BCOMPARE__RESULTS
#define MERGETOOL__BCOMPARE__RESULTS__LINUX MERGETOOL__BCOMPARE__RESULTS

// Difftool: SourceGear DiffMerge (diffmerge)
#define DIFFTOOL__DIFFMERGE "diffmerge"
const char* DIFFTOOL__DIFFMERGE__PATHS__WINDOWS[] =
{
	// 3.3.1+ MSM-based common
	"C:\\Program Files\\SourceGear\\Common\\DiffMerge\\sgdm.exe",
	"C:\\Program Files (x86)\\SourceGear\\Common\\DiffMerge\\sgdm.exe",

	// 3.3.0 and earlier stand-alone
	"C:\\Program Files\\SourceGear\\DiffMerge\\DiffMerge.exe",
	"C:\\Program Files (x86)\\SourceGear\\DiffMerge\\DiffMerge.exe",

	// bundled versions with other products
	"C:\\Program Files\\SourceGear\\Fortress Client\\sgdm.exe",
	"C:\\Program Files (x86)\\SourceGear\\Fortress Client\\sgdm.exe",
	"C:\\Program Files\\SourceGear\\Vault Client\\sgdm.exe",
	"C:\\Program Files (x86)\\SourceGear\\Vault Client\\sgdm.exe",
	"C:\\Program Files\\SourceGear\\VaultPro Client\\sgdm.exe",
	"C:\\Program Files (x86)\\SourceGear\\VaultPro Client\\sgdm.exe",

	// PATH
	"sgdm.exe",

	NULL
};
const char* DIFFTOOL__DIFFMERGE__PATHS__LINUX[] =
{
	"/usr/local/bin/diffmerge",
	"/usr/bin/diffmerge",
	"diffmerge",
	NULL
};
const char* DIFFTOOL__DIFFMERGE__PATHS__MAC[] =
{
	"/Applications/DiffMerge.app/Contents/Resources/diffmerge.sh",
	"/usr/local/bin/diffmerge",
	"/usr/bin/diffmerge",
	"diffmerge",
	NULL
};
const char* DIFFTOOL__DIFFMERGE__ARGS[] =
{
	"-nosplash",
	"@TO_WRITABLE||-ro2@",
	"-t1",
	"@FROM_LABEL@",
	"-t2",
	"@TO_LABEL@",
	"@FROM@",
	"@TO@",
	NULL
};
#define DIFFTOOL__DIFFMERGE__ARGS__WINDOWS DIFFTOOL__DIFFMERGE__ARGS
#define DIFFTOOL__DIFFMERGE__ARGS__LINUX DIFFTOOL__DIFFMERGE__ARGS
#define DIFFTOOL__DIFFMERGE__ARGS__MAC DIFFTOOL__DIFFMERGE__ARGS

// Mergetool: SourceGear DiffMerge (diffmerge)
#define MERGETOOL__DIFFMERGE DIFFTOOL__DIFFMERGE
#define MERGETOOL__DIFFMERGE__PATHS__WINDOWS DIFFTOOL__DIFFMERGE__PATHS__WINDOWS
#define MERGETOOL__DIFFMERGE__PATHS__LINUX DIFFTOOL__DIFFMERGE__PATHS__LINUX
#define MERGETOOL__DIFFMERGE__PATHS__MAC DIFFTOOL__DIFFMERGE__PATHS__MAC
const char* MERGETOOL__DIFFMERGE__ARGS[] =
{
	"-nosplash",
	"-merge",
	"-t1",
	"@BASELINE_LABEL@",
	"-t2",
	"@RESULT_LABEL@",
	"-t3",
	"@OTHER_LABEL@",
	"-result",
	"@RESULT@",
	"@BASELINE@",
	"@ANCESTOR@",
	"@OTHER@",
	NULL
};
#define MERGETOOL__DIFFMERGE__ARGS__WINDOWS MERGETOOL__DIFFMERGE__ARGS
#define MERGETOOL__DIFFMERGE__ARGS__LINUX MERGETOOL__DIFFMERGE__ARGS
#define MERGETOOL__DIFFMERGE__ARGS__MAC MERGETOOL__DIFFMERGE__ARGS
const char* MERGETOOL__DIFFMERGE__EXITS[] =
{
	"1", // user chose "abort" when exiting with conflicts remaining
	NULL
};
#define MERGETOOL__DIFFMERGE__EXITS__WINDOWS MERGETOOL__DIFFMERGE__EXITS
#define MERGETOOL__DIFFMERGE__EXITS__LINUX MERGETOOL__DIFFMERGE__EXITS
#define MERGETOOL__DIFFMERGE__EXITS__MAC MERGETOOL__DIFFMERGE__EXITS
const char* MERGETOOL__DIFFMERGE__RESULTS[] =
{
	SG_MERGETOOL__RESULT__CANCEL__SZ,
	NULL
};
#define MERGETOOL__DIFFMERGE__RESULTS__WINDOWS MERGETOOL__DIFFMERGE__RESULTS
#define MERGETOOL__DIFFMERGE__RESULTS__LINUX MERGETOOL__DIFFMERGE__RESULTS
#define MERGETOOL__DIFFMERGE__RESULTS__MAC MERGETOOL__DIFFMERGE__RESULTS

// Difftool: Diffuse (diffuse)
#define DIFFTOOL__DIFFUSE "diffuse"
const char* DIFFTOOL__DIFFUSE__PATHS__WINDOWS[] =
{
	"C:\\Program Files\\Diffuse\\diffuse.exe",
	"C:\\Program Files (x86)\\Diffuse\\diffuse.exe",
	"diffuse.exe",
	NULL
};
const char* DIFFTOOL__DIFFUSE__PATHS__LINUX[] =
{
	"/usr/local/bin/diffuse",
	"/usr/bin/diffuse",
	"diffuse",
	NULL
};
#define DIFFTOOL__DIFFUSE__PATHS__MAC DIFFTOOL__DIFFUSE__PATHS__LINUX
const char* DIFFTOOL__DIFFUSE__ARGS[] =
{
	"@FROM@",
	"@TO@",
	NULL
};
#define DIFFTOOL__DIFFUSE__ARGS__WINDOWS DIFFTOOL__DIFFUSE__ARGS
#define DIFFTOOL__DIFFUSE__ARGS__LINUX DIFFTOOL__DIFFUSE__ARGS
#define DIFFTOOL__DIFFUSE__ARGS__MAC DIFFTOOL__DIFFUSE__ARGS

// Mergetool: Diffuse (diffuse)
#define MERGETOOL__DIFFUSE DIFFTOOL__DIFFUSE
#define MERGETOOL__DIFFUSE__PATHS__WINDOWS DIFFTOOL__DIFFUSE__PATHS__WINDOWS
#define MERGETOOL__DIFFUSE__PATHS__LINUX DIFFTOOL__DIFFUSE__PATHS__LINUX
#define MERGETOOL__DIFFUSE__PATHS__MAC DIFFTOOL__DIFFUSE__PATHS__MAC
const char* MERGETOOL__DIFFUSE__ARGS[] =
{
	"@BASELINE@",
	"@RESULT@",
	"@OTHER@",
	NULL
};
#define MERGETOOL__DIFFUSE__ARGS__WINDOWS MERGETOOL__DIFFUSE__ARGS
#define MERGETOOL__DIFFUSE__ARGS__LINUX MERGETOOL__DIFFUSE__ARGS
#define MERGETOOL__DIFFUSE__ARGS__MAC MERGETOOL__DIFFUSE__ARGS
const char* MERGETOOL__DIFFUSE__FLAGS[] =
{
	SG_MERGETOOL__FLAG__ANCESTOR_OVERWRITE,
	NULL
};
#define MERGETOOL__DIFFUSE__FLAGS__WINDOWS MERGETOOL__DIFFUSE__FLAGS
#define MERGETOOL__DIFFUSE__FLAGS__LINUX MERGETOOL__DIFFUSE__FLAGS
#define MERGETOOL__DIFFUSE__FLAGS__MAC MERGETOOL__DIFFUSE__FLAGS

// Difftool: Diffuse 0.4.5+ (diffuse-0.4.5)
#define DIFFTOOL__DIFFUSE_045 "diffuse-0.4.5"
#define DIFFTOOL__DIFFUSE_045__PATHS__WINDOWS DIFFTOOL__DIFFUSE__PATHS__WINDOWS
#define DIFFTOOL__DIFFUSE_045__PATHS__LINUX DIFFTOOL__DIFFUSE__PATHS__LINUX
#define DIFFTOOL__DIFFUSE_045__PATHS__MAC DIFFTOOL__DIFFUSE__PATHS__MAC
const char* DIFFTOOL__DIFFUSE_045__ARGS[] =
{
	"-L",
	"@FROM_LABEL@",
	"@FROM@",
	"-L",
	"@TO_LABEL@",
	"@TO@",
	NULL
};
#define DIFFTOOL__DIFFUSE_045__ARGS__WINDOWS DIFFTOOL__DIFFUSE_045__ARGS
#define DIFFTOOL__DIFFUSE_045__ARGS__LINUX DIFFTOOL__DIFFUSE_045__ARGS
#define DIFFTOOL__DIFFUSE_045__ARGS__MAC DIFFTOOL__DIFFUSE_045__ARGS

// Mergetool: Diffuse 0.4.5+ (diffuse-0.4.5)
#define MERGETOOL__DIFFUSE_045 DIFFTOOL__DIFFUSE_045
#define MERGETOOL__DIFFUSE_045__PATHS__WINDOWS DIFFTOOL__DIFFUSE_045__PATHS__WINDOWS
#define MERGETOOL__DIFFUSE_045__PATHS__LINUX DIFFTOOL__DIFFUSE_045__PATHS__LINUX
#define MERGETOOL__DIFFUSE_045__PATHS__MAC DIFFTOOL__DIFFUSE_045__PATHS__MAC
const char* MERGETOOL__DIFFUSE_045__ARGS[] =
{
	"-L",
	"@BASELINE_LABEL@",
	"@BASELINE@",
	"-L",
	"@RESULT_LABEL@",
	"@RESULT@",
	"-L",
	"@OTHER_LABEL@",
	"@OTHER@",
	NULL
};
#define MERGETOOL__DIFFUSE_045__ARGS__WINDOWS MERGETOOL__DIFFUSE_045__ARGS
#define MERGETOOL__DIFFUSE_045__ARGS__LINUX MERGETOOL__DIFFUSE_045__ARGS
#define MERGETOOL__DIFFUSE_045__ARGS__MAC MERGETOOL__DIFFUSE_045__ARGS
#define MERGETOOL__DIFFUSE_045__FLAGS MERGETOOL__DIFFUSE__FLAGS
#define MERGETOOL__DIFFUSE_045__FLAGS__WINDOWS MERGETOOL__DIFFUSE_045__FLAGS
#define MERGETOOL__DIFFUSE_045__FLAGS__LINUX MERGETOOL__DIFFUSE_045__FLAGS
#define MERGETOOL__DIFFUSE_045__FLAGS__MAC MERGETOOL__DIFFUSE_045__FLAGS

// Difftool: Ellie Computing Merge (ecmerge)
#define DIFFTOOL__ECMERGE "ecmerge"
const char* DIFFTOOL__ECMERGE__PATHS__WINDOWS[] =
{
	"C:\\Program Files\\Ellie Computing\\Merge\\ecmerge.exe",
	"C:\\Program Files (x86)\\Ellie Computing\\Merge\\ecmerge.exe",
	// Note: "c3 a9" is the UTF-8 encoding of U+00E9: LATIN SMALL LETTER E WITH ACUTE
	"C:\\Program Files\\Elli\xc3\xa9 Computing\\Merge\\ecmerge.exe",
	"C:\\Program Files (x86)\\Elli\xc3\xa9 Computing\\Merge\\ecmerge.exe",
	"C:\\Program Files\\Elli\xc3\xa9 Computing\\Merge\\guimerge.exe",
	"C:\\Program Files (x86)\\Elli\xc3\xa9 Computing\\Merge\\guimerge.exe",
	"ecmerge.exe", // executable name changed with version 2.4
	NULL
};
const char* DIFFTOOL__ECMERGE__PATHS__LINUX[] =
{
	"/usr/local/bin/ecmerge",
	"/usr/bin/ecmerge",
	"ecmerge",
	NULL
};
const char* DIFFTOOL__ECMERGE__PATHS__MAC[] =
{
	"/Applications/ECMerge.app/Contents/MacOS/ecmerge",
	"/Applications/Utilities/ECMerge.app/Contents/MacOS/ecmerge",
	"/Developer/Applications/ECMerge.app/Contents/MacOS/ecmerge",
	"/Developer/Applications/Utilities/ECMerge.app/Contents/MacOS/ecmerge",
	"/Applications/ECMerge.app/Contents/MacOS/guimerge",
	"/Applications/Utilities/ECMerge.app/Contents/MacOS/guimerge",
	"/Developer/Applications/ECMerge.app/Contents/MacOS/guimerge",
	"/Developer/Applications/Utilities/ECMerge.app/Contents/MacOS/guimerge",
	"/usr/local/bin/ecmerge",
	"/usr/bin/ecmerge",
	"ecmerge", // executable name changed with version 2.4?
	NULL
};
const char* DIFFTOOL__ECMERGE__ARGS[] =
{
	"--mode=diff2",
	"--title1=@FROM_LABEL@",
	"--flags1=no_mru,read_only",
	"--title2=@TO_LABEL@",
	"--flags2=no_mru@TO_WRITABLE||,read_only@",
	"@FROM@",
	"@TO@",
	NULL
};
#define DIFFTOOL__ECMERGE__ARGS__WINDOWS DIFFTOOL__ECMERGE__ARGS
#define DIFFTOOL__ECMERGE__ARGS__LINUX DIFFTOOL__ECMERGE__ARGS
#define DIFFTOOL__ECMERGE__ARGS__MAC DIFFTOOL__ECMERGE__ARGS

// Mergetool: Ellie Computing Merge (ecmerge)
#define MERGETOOL__ECMERGE DIFFTOOL__ECMERGE
#define MERGETOOL__ECMERGE__PATHS__WINDOWS DIFFTOOL__ECMERGE__PATHS__WINDOWS
#define MERGETOOL__ECMERGE__PATHS__LINUX DIFFTOOL__ECMERGE__PATHS__LINUX
#define MERGETOOL__ECMERGE__PATHS__MAC DIFFTOOL__ECMERGE__PATHS__MAC
const char* MERGETOOL__ECMERGE__ARGS[] =
{
	"--mode=merge3",
	"--title0=@ANCESTOR_LABEL@",
	"--flags0=no_mru,read_only",
	"--title1=@BASELINE_LABEL@",
	"--flags1=no_mru,read_only",
	"--title2=@OTHER_LABEL@",
	"--flags2=no_mru,read_only",
	"--to=@RESULT@",
	"--to-title=@RESULT_LABEL@",
	"--to-flags=no_mru",
	"@ANCESTOR@",
	"@BASELINE@",
	"@OTHER@",
	NULL
};
#define MERGETOOL__ECMERGE__ARGS__WINDOWS MERGETOOL__ECMERGE__ARGS
#define MERGETOOL__ECMERGE__ARGS__LINUX MERGETOOL__ECMERGE__ARGS
#define MERGETOOL__ECMERGE__ARGS__MAC MERGETOOL__ECMERGE__ARGS
const char* MERGETOOL__ECMERGE__EXITS[] =
{
	"1", // seems to always exit with this code, no matter what
	NULL
};
#define MERGETOOL__ECMERGE__EXITS__WINDOWS MERGETOOL__ECMERGE__EXITS
#define MERGETOOL__ECMERGE__EXITS__LINUX MERGETOOL__ECMERGE__EXITS
#define MERGETOOL__ECMERGE__EXITS__MAC MERGETOOL__ECMERGE__EXITS
const char* MERGETOOL__ECMERGE__RESULTS[] =
{
	SG_FILETOOL__RESULT__SUCCESS__SZ,
	NULL
};
#define MERGETOOL__ECMERGE__RESULTS__WINDOWS MERGETOOL__ECMERGE__RESULTS
#define MERGETOOL__ECMERGE__RESULTS__LINUX MERGETOOL__ECMERGE__RESULTS
#define MERGETOOL__ECMERGE__RESULTS__MAC MERGETOOL__ECMERGE__RESULTS

// Difftool: KDiff3 (kdiff3)
#define DIFFTOOL__KDIFF3 "kdiff3"
const char* DIFFTOOL__KDIFF3__PATHS__WINDOWS[] =
{
	"C:\\Program Files\\KDiff3\\kdiff3.exe",
	"C:\\Program Files (x86)\\KDiff3\\kdiff3.exe",
	"kdiff3.exe",
	NULL
};
const char* DIFFTOOL__KDIFF3__PATHS__LINUX[] =
{
	"/usr/local/bin/kdiff3",
	"/usr/bin/kdiff3",
	"kdiff3",
	NULL
};
const char* DIFFTOOL__KDIFF3__PATHS__MAC[] =
{
	"/Applications/kdiff3.app/Contents/MacOS/kdiff3",
	"/Applications/Utilities/kdiff3.app/Contents/MacOS/kdiff3",
	"/Developer/Applications/kdiff3.app/Contents/MacOS/kdiff3",
	"/Developer/Applications/Utilities/kdiff3.app/Contents/MacOS/kdiff3",
	"/usr/local/bin/kdiff3",
	"/usr/bin/kdiff3",
	"kdiff3",
	NULL
};
const char* DIFFTOOL__KDIFF3__ARGS[] =
{
	"--L1",
	"@FROM_LABEL@",
	"--L2",
	"@TO_LABEL@",
	"@FROM@",
	"@TO@",
	NULL
};
#define DIFFTOOL__KDIFF3__ARGS__WINDOWS DIFFTOOL__KDIFF3__ARGS
#define DIFFTOOL__KDIFF3__ARGS__LINUX DIFFTOOL__KDIFF3__ARGS
#define DIFFTOOL__KDIFF3__ARGS__MAC DIFFTOOL__KDIFF3__ARGS
const char* DIFFTOOL__KDIFF3__EXITS[] =
{
	"1", // user didn't save anything
	NULL
};
#define DIFFTOOL__KDIFF3__EXITS__WINDOWS DIFFTOOL__KDIFF3__EXITS
#define DIFFTOOL__KDIFF3__EXITS__LINUX DIFFTOOL__KDIFF3__EXITS
#define DIFFTOOL__KDIFF3__EXITS__MAC DIFFTOOL__KDIFF3__EXITS
const char* DIFFTOOL__KDIFF3__RESULTS[] =
{
	SG_FILETOOL__RESULT__SUCCESS__SZ,
	NULL
};
#define DIFFTOOL__KDIFF3__RESULTS__WINDOWS DIFFTOOL__KDIFF3__RESULTS
#define DIFFTOOL__KDIFF3__RESULTS__LINUX DIFFTOOL__KDIFF3__RESULTS
#define DIFFTOOL__KDIFF3__RESULTS__MAC DIFFTOOL__KDIFF3__RESULTS

// Mergetool: KDiff3 (kdiff3)
#define MERGETOOL__KDIFF3 DIFFTOOL__KDIFF3
#define MERGETOOL__KDIFF3__PATHS__WINDOWS DIFFTOOL__KDIFF3__PATHS__WINDOWS
#define MERGETOOL__KDIFF3__PATHS__LINUX DIFFTOOL__KDIFF3__PATHS__LINUX
#define MERGETOOL__KDIFF3__PATHS__MAC DIFFTOOL__KDIFF3__PATHS__MAC
const char* MERGETOOL__KDIFF3__ARGS[] =
{
	"--merge",
	"--L1",
	"@ANCESTOR_LABEL@",
	"--L2",
	"@BASELINE_LABEL@",
	"--L3",
	"@OTHER_LABEL@",
	"--output",
	"@RESULT@",
	"@ANCESTOR@",
	"@BASELINE@",
	"@OTHER@",
	NULL
};
#define MERGETOOL__KDIFF3__ARGS__WINDOWS MERGETOOL__KDIFF3__ARGS
#define MERGETOOL__KDIFF3__ARGS__LINUX MERGETOOL__KDIFF3__ARGS
#define MERGETOOL__KDIFF3__ARGS__MAC MERGETOOL__KDIFF3__ARGS
const char* MERGETOOL__KDIFF3__EXITS[] =
{
	"1", // user didn't save anything (Note: kdiff3 won't even allow saving when conflicts remain.)
	NULL
};
#define MERGETOOL__KDIFF3__EXITS__WINDOWS MERGETOOL__KDIFF3__EXITS
#define MERGETOOL__KDIFF3__EXITS__LINUX MERGETOOL__KDIFF3__EXITS
#define MERGETOOL__KDIFF3__EXITS__MAC MERGETOOL__KDIFF3__EXITS
const char* MERGETOOL__KDIFF3__RESULTS[] =
{
	SG_MERGETOOL__RESULT__CANCEL__SZ,
	NULL
};
#define MERGETOOL__KDIFF3__RESULTS__WINDOWS MERGETOOL__KDIFF3__RESULTS
#define MERGETOOL__KDIFF3__RESULTS__LINUX MERGETOOL__KDIFF3__RESULTS
#define MERGETOOL__KDIFF3__RESULTS__MAC MERGETOOL__KDIFF3__RESULTS

// Difftool: Meld (meld)
#define DIFFTOOL__MELD "meld"
const char* DIFFTOOL__MELD__PATHS__LINUX[] =
{
	"/usr/local/bin/meld",
	"/usr/bin/meld",
	"meld",
	NULL
};
#define DIFFTOOL__MELD__PATHS__MAC DIFFTOOL__MELD__PATHS__LINUX
const char* DIFFTOOL__MELD__ARGS[] =
{
	"--label=@FROM_LABEL@",
	"@FROM@",
	"--label=@TO_LABEL@",
	"@TO@",
	NULL
};
#define DIFFTOOL__MELD__ARGS__LINUX DIFFTOOL__MELD__ARGS
#define DIFFTOOL__MELD__ARGS__MAC DIFFTOOL__MELD__ARGS

// Mergetool: Meld (meld)
#define MERGETOOL__MELD DIFFTOOL__MELD
#define MERGETOOL__MELD__PATHS__LINUX DIFFTOOL__MELD__PATHS__LINUX
#define MERGETOOL__MELD__PATHS__MAC DIFFTOOL__MELD__PATHS__MAC
const char* MERGETOOL__MELD__ARGS[] =
{
	"--label=@BASELINE_LABEL@",
	"@BASELINE@",
	"--label=@RESULT_LABEL@",
	"@RESULT@",
	"--label=@OTHER_LABEL@",
	"@OTHER@",
	NULL
};
#define MERGETOOL__MELD__ARGS__LINUX MERGETOOL__MELD__ARGS
#define MERGETOOL__MELD__ARGS__MAC MERGETOOL__MELD__ARGS
const char* MERGETOOL__MELD__FLAGS[] =
{
	SG_MERGETOOL__FLAG__ANCESTOR_OVERWRITE,
	NULL
};
#define MERGETOOL__MELD__FLAGS__LINUX MERGETOOL__MELD__FLAGS
#define MERGETOOL__MELD__FLAGS__MAC MERGETOOL__MELD__FLAGS

// Mergetool: Meld 1.3.2+ (meld-132)
#define MERGETOOL__MELD_132 "meld-1.3.2"
#define MERGETOOL__MELD_132__PATHS__LINUX MERGETOOL__MELD__PATHS__LINUX
#define MERGETOOL__MELD_132__PATHS__MAC MERGETOOL__MELD__PATHS__MAC
const char* MERGETOOL__MELD_132__ARGS[] =
{
	"--label=@BASELINE_LABEL@",
	"@BASELINE@",
	"--label=@RESULT_LABEL@",
	"@ANCESTOR@",
	"--label=@OTHER_LABEL@",
	"@OTHER@",
	"@RESULT@",
	NULL
};
#define MERGETOOL__MELD_132__ARGS__LINUX MERGETOOL__MELD_132__ARGS
#define MERGETOOL__MELD_132__ARGS__MAC MERGETOOL__MELD_132__ARGS

// Difftool: Perforce Merge (p4merge)
#define DIFFTOOL__P4MERGE "p4merge"
const char* DIFFTOOL__P4MERGE__PATHS__WINDOWS[] =
{
	"C:\\Program Files\\Perforce\\p4merge.exe",
	"C:\\Program Files (x86)\\Perforce\\p4merge.exe",
	"p4merge.exe",
	NULL
};
const char* DIFFTOOL__P4MERGE__PATHS__LINUX[] =
{
	"/usr/local/bin/p4merge",
	"/usr/bin/p4merge",
	"p4merge",
	NULL
};
const char* DIFFTOOL__P4MERGE__PATHS__MAC[] =
{
	"/Applications/p4merge.app/Contents/Resources/launchp4merge",
	"/Applications/Utilities/p4merge.app/Contents/Resources/launchp4merge",
	"/Developer/Applications/p4merge.app/Contents/Resources/launchp4merge",
	"/Developer/Applications/Utilities/p4merge.app/Contents/Resources/launchp4merge",
	NULL
};
const char* DIFFTOOL__P4MERGE__ARGS[] =
{
	"-nl",
	"@FROM_LABEL@",
	"-nr",
	"@TO_LABEL@",
	"@FROM@",
	"@TO@",
	NULL
};
#define DIFFTOOL__P4MERGE__ARGS__WINDOWS DIFFTOOL__P4MERGE__ARGS
#define DIFFTOOL__P4MERGE__ARGS__LINUX DIFFTOOL__P4MERGE__ARGS
#define DIFFTOOL__P4MERGE__ARGS__MAC DIFFTOOL__P4MERGE__ARGS
const char* DIFFTOOL__P4MERGE__FLAGS__MAC[] =
{
	SG_FILETOOL__FLAG__SYSTEM_CALL,
	NULL
};

// Mergetool: Perforce Merge (p4merge)
#define MERGETOOL__P4MERGE DIFFTOOL__P4MERGE
#define MERGETOOL__P4MERGE__PATHS__WINDOWS DIFFTOOL__P4MERGE__PATHS__WINDOWS
#define MERGETOOL__P4MERGE__PATHS__LINUX DIFFTOOL__P4MERGE__PATHS__LINUX
#define MERGETOOL__P4MERGE__PATHS__MAC DIFFTOOL__P4MERGE__PATHS__MAC
const char* MERGETOOL__P4MERGE__ARGS[] =
{
	"-nb",
	"@ANCESTOR_LABEL@",
	"-nl",
	"@BASELINE_LABEL@",
	"-nr",
	"@OTHER_LABEL@",
	"-nm",
	"@RESULT_LABEL@",
	"@ANCESTOR@",
	"@BASELINE@",
	"@OTHER@",
	"@RESULT@",
	NULL
};
#define MERGETOOL__P4MERGE__ARGS__WINDOWS MERGETOOL__P4MERGE__ARGS
#define MERGETOOL__P4MERGE__ARGS__LINUX MERGETOOL__P4MERGE__ARGS
#define MERGETOOL__P4MERGE__ARGS__MAC MERGETOOL__P4MERGE__ARGS
const char* MERGETOOL__P4MERGE__FLAGS__WINDOWS[] =
{
	SG_MERGETOOL__FLAG__PREMAKE_RESULT,
	NULL
};
const char* MERGETOOL__P4MERGE__FLAGS__LINUX[] =
{
	SG_MERGETOOL__FLAG__PREMAKE_RESULT,
	NULL
};
const char* MERGETOOL__P4MERGE__FLAGS__MAC[] =
{
	SG_MERGETOOL__FLAG__PREMAKE_RESULT,
	SG_FILETOOL__FLAG__SYSTEM_CALL,
	NULL
};

// Difftool: TkDiff (tkdiff)
#define DIFFTOOL__TKDIFF "tkdiff"
const char* DIFFTOOL__TKDIFF__PATHS__WINDOWS[] =
{
	"C:\\Program Files\\TkDiff\\tkdiff.exe",
	"C:\\Program Files (x86)\\TkDiff\\tkdiff.exe",
	"tkdiff.exe",
	NULL
};
const char* DIFFTOOL__TKDIFF__PATHS__LINUX[] =
{
	"/usr/local/bin/tkdiff",
	"/usr/bin/tkdiff",
	"tkdiff",
	NULL
};
const char* DIFFTOOL__TKDIFF__PATHS__MAC[] =
{
	"/Applications/TkDiff.app/Contents/MacOS/Wish Shell",
	"/Applications/Utilities/TkDiff.app/Contents/MacOS/Wish Shell",
	"/Developer/Applications/TkDiff.app/Contents/MacOS/Wish Shell",
	"/Developer/Applications/Utilities/TkDiff.app/Contents/MacOS/Wish Shell",
	NULL
};
const char* DIFFTOOL__TKDIFF__ARGS[] =
{
	"-L",
	"@FROM_LABEL@",
	"-L",
	"@TO_LABEL@",
	"@FROM@",
	"@TO@",
	NULL
};
#define DIFFTOOL__TKDIFF__ARGS__WINDOWS DIFFTOOL__TKDIFF__ARGS
#define DIFFTOOL__TKDIFF__ARGS__LINUX DIFFTOOL__TKDIFF__ARGS
#define DIFFTOOL__TKDIFF__ARGS__MAC DIFFTOOL__TKDIFF__ARGS
const char* DIFFTOOL__TKDIFF__EXITS[] =
{
	"1", // files are different
	NULL
};
#define DIFFTOOL__TKDIFF__EXITS__WINDOWS DIFFTOOL__TKDIFF__EXITS
#define DIFFTOOL__TKDIFF__EXITS__LINUX DIFFTOOL__TKDIFF__EXITS
#define DIFFTOOL__TKDIFF__EXITS__MAC DIFFTOOL__TKDIFF__EXITS
const char* DIFFTOOL__TKDIFF__RESULTS[] =
{
	SG_DIFFTOOL__RESULT__DIFFERENT__SZ,
	NULL
};
#define DIFFTOOL__TKDIFF__RESULTS__WINDOWS DIFFTOOL__TKDIFF__RESULTS
#define DIFFTOOL__TKDIFF__RESULTS__LINUX DIFFTOOL__TKDIFF__RESULTS
#define DIFFTOOL__TKDIFF__RESULTS__MAC DIFFTOOL__TKDIFF__RESULTS

// Mergetool: TkDiff (tkdiff)
#define MERGETOOL__TKDIFF DIFFTOOL__TKDIFF
#define MERGETOOL__TKDIFF__PATHS__WINDOWS DIFFTOOL__TKDIFF__PATHS__WINDOWS
#define MERGETOOL__TKDIFF__PATHS__LINUX DIFFTOOL__TKDIFF__PATHS__LINUX
#define MERGETOOL__TKDIFF__PATHS__MAC DIFFTOOL__TKDIFF__PATHS__MAC
const char* MERGETOOL__TKDIFF__ARGS[] =
{
	"-L",
	"@BASELINE_LABEL@",
	"-L",
	"@OTHER_LABEL@",
	"-a",
	"@ANCESTOR@",
	"-o",
	"@RESULT@",
	"@BASELINE@",
	"@OTHER@",
	NULL
};
#define MERGETOOL__TKDIFF__ARGS__WINDOWS MERGETOOL__TKDIFF__ARGS
#define MERGETOOL__TKDIFF__ARGS__LINUX MERGETOOL__TKDIFF__ARGS
#define MERGETOOL__TKDIFF__ARGS__MAC MERGETOOL__TKDIFF__ARGS
const char* MERGETOOL__TKDIFF__EXITS[] =
{
	"1", // seems to indicate that the user closed the main window instead of exiting via the Merge Preview window
	NULL
};
#define MERGETOOL__TKDIFF__EXITS__WINDOWS MERGETOOL__TKDIFF__EXITS
#define MERGETOOL__TKDIFF__EXITS__LINUX MERGETOOL__TKDIFF__EXITS
#define MERGETOOL__TKDIFF__EXITS__MAC MERGETOOL__TKDIFF__EXITS
const char* MERGETOOL__TKDIFF__RESULTS[] =
{
	SG_FILETOOL__RESULT__SUCCESS__SZ,
	NULL
};
#define MERGETOOL__TKDIFF__RESULTS__WINDOWS MERGETOOL__TKDIFF__RESULTS
#define MERGETOOL__TKDIFF__RESULTS__LINUX MERGETOOL__TKDIFF__RESULTS
#define MERGETOOL__TKDIFF__RESULTS__MAC MERGETOOL__TKDIFF__RESULTS

// Difftool: TortoiseSVN Merge (tortoisemerge)
#define DIFFTOOL__TORTOISEMERGE "tortoisemerge"
const char* DIFFTOOL__TORTOISEMERGE__PATHS__WINDOWS[] =
{
	"C:\\Program Files\\TortoiseSVN\\bin\\TortoiseMerge.exe",
	"C:\\Program Files (x86)\\TortoiseSVN\\bin\\TortoiseMerge.exe",
	"TortoiseMerge.exe",
	NULL
};
const char* DIFFTOOL__TORTOISEMERGE__ARGS__WINDOWS[] =
{
	// Note: As of 1.7.4, /readonly doesn't seem to do anything.
	// TortoiseMerge treats /base as read-only, and /mine as writable, regardless.
	// It accepts /readonly:filename as well, but it still has no effect regardless of specified filename.
	// Placement of /readonly relative to other params/switches also doesn't seem to make a difference.
	"/base:@FROM@",
	"/basename:@FROM_LABEL@",
	"/mine:@TO@",
	"/minename:@TO_LABEL@",
	NULL
};

// Mergetool: TortoiseSVN Merge (tortoisemerge)
#define MERGETOOL__TORTOISEMERGE DIFFTOOL__TORTOISEMERGE
#define MERGETOOL__TORTOISEMERGE__PATHS__WINDOWS DIFFTOOL__TORTOISEMERGE__PATHS__WINDOWS
#define MERGETOOL__TORTOISEMERGE__PATHS__LINUX DIFFTOOL__TORTOISEMERGE__PATHS__LINUX
#define MERGETOOL__TORTOISEMERGE__PATHS__MAC DIFFTOOL__TORTOISEMERGE__PATHS__MAC
const char* MERGETOOL__TORTOISEMERGE__ARGS__WINDOWS[] =
{
	// Note: As of 1.7.4, /readonly doesn't seem to do anything.
	// TortoiseMerge treats /mine and /theirs as read-only, and /merged as writable, regardless.
	// The /base file isn't displayed at all, and thus couldn't be edited anyway.
	// It accepts /readonly:filename as well, but it still has no effect regardless of specified filename.
	// Placement of /readonly relative to other params/switches also doesn't seem to make a difference.
	"/base:@ANCESTOR@",
	"/basename:@ANCESTOR_LABEL@",
	"/mine:@BASELINE@",
	"/minename:@BASELINE_LABEL@",
	"/theirs:@OTHER@",
	"/theirsname:@OTHER_LABEL@",
	"/merged:@RESULT@",
	"/mergedname:@RESULT_LABEL@",
	NULL
};

// Difftool: Vim (vimdiff)
#define DIFFTOOL__VIMDIFF "vimdiff"
const char* DIFFTOOL__VIMDIFF__PATHS__WINDOWS[] =
{
	"C:\\Program Files\\Vim\\vim73\\vim.exe",
	"C:\\Program Files (x86)\\Vim\\vim73\\vim.exe",
	"C:\\Program Files\\Vim\\vim72\\vim.exe",
	"C:\\Program Files (x86)\\Vim\\vim72\\vim.exe",
	"C:\\Program Files\\Vim\\vim71\\vim.exe",
	"C:\\Program Files (x86)\\Vim\\vim71\\vim.exe",
	"C:\\Program Files\\Vim\\vim70\\vim.exe",
	"C:\\Program Files (x86)\\Vim\\vim70\\vim.exe",
	"C:\\Program Files\\Vim\\vim7\\vim.exe",
	"C:\\Program Files (x86)\\Vim\\vim7\\vim.exe",
	"vim.exe",
	NULL
};
const char* DIFFTOOL__VIMDIFF__PATHS__LINUX[] =
{
	"/usr/local/bin/vim",
	"/usr/bin/vim",
	"vim",
	NULL
};
#define DIFFTOOL__VIMDIFF__PATHS__MAC DIFFTOOL__VIMDIFF__PATHS__LINUX
const char* DIFFTOOL__VIMDIFF__ARGS[] =
{
	"-d",
	"-O",
	"@FROM@",
	"@TO@",
	NULL
};
#define DIFFTOOL__VIMDIFF__ARGS__WINDOWS DIFFTOOL__VIMDIFF__ARGS
#define DIFFTOOL__VIMDIFF__ARGS__LINUX DIFFTOOL__VIMDIFF__ARGS
#define DIFFTOOL__VIMDIFF__ARGS__MAC DIFFTOOL__VIMDIFF__ARGS

// Mergetool: Vim (vimdiff)
#define MERGETOOL__VIMDIFF DIFFTOOL__VIMDIFF
#define MERGETOOL__VIMDIFF__PATHS__WINDOWS DIFFTOOL__VIMDIFF__PATHS__WINDOWS
#define MERGETOOL__VIMDIFF__PATHS__LINUX DIFFTOOL__VIMDIFF__PATHS__LINUX
#define MERGETOOL__VIMDIFF__PATHS__MAC DIFFTOOL__VIMDIFF__PATHS__MAC
const char* MERGETOOL__VIMDIFF__ARGS[] =
{
	"-d",
	"-O",
	"@BASELINE@",
	"@RESULT@",
	"@OTHER@",
	NULL
};
#define MERGETOOL__VIMDIFF__ARGS__WINDOWS MERGETOOL__VIMDIFF__ARGS
#define MERGETOOL__VIMDIFF__ARGS__LINUX MERGETOOL__VIMDIFF__ARGS
#define MERGETOOL__VIMDIFF__ARGS__MAC MERGETOOL__VIMDIFF__ARGS
const char* MERGETOOL__VIMDIFF__FLAGS[] =
{
	SG_MERGETOOL__FLAG__ANCESTOR_OVERWRITE,
	NULL
};
#define MERGETOOL__VIMDIFF__FLAGS__WINDOWS MERGETOOL__VIMDIFF__FLAGS
#define MERGETOOL__VIMDIFF__FLAGS__LINUX MERGETOOL__VIMDIFF__FLAGS
#define MERGETOOL__VIMDIFF__FLAGS__MAC MERGETOOL__VIMDIFF__FLAGS

// Difftool: Vim GUI (gvimdiff)
#define DIFFTOOL__GVIMDIFF "gvimdiff"
const char* DIFFTOOL__GVIMDIFF__PATHS__WINDOWS[] =
{
	"C:\\Program Files\\Vim\\vim73\\gvim.exe",
	"C:\\Program Files (x86)\\Vim\\vim73\\gvim.exe",
	"C:\\Program Files\\Vim\\vim72\\gvim.exe",
	"C:\\Program Files (x86)\\Vim\\vim72\\gvim.exe",
	"C:\\Program Files\\Vim\\vim71\\gvim.exe",
	"C:\\Program Files (x86)\\Vim\\vim71\\gvim.exe",
	"C:\\Program Files\\Vim\\vim70\\gvim.exe",
	"C:\\Program Files (x86)\\Vim\\vim70\\gvim.exe",
	"C:\\Program Files\\Vim\\vim7\\gvim.exe",
	"C:\\Program Files (x86)\\Vim\\vim7\\gvim.exe",
	"gvim.exe",
	NULL
};
const char* DIFFTOOL__GVIMDIFF__PATHS__LINUX[] =
{
	"/usr/local/bin/vim",
	"/usr/bin/vim",
	"vim",
	NULL
};
#define DIFFTOOL__GVIMDIFF__PATHS__MAC DIFFTOOL__GVIMDIFF__PATHS__LINUX
const char* DIFFTOOL__GVIMDIFF__ARGS__WINDOWS[] =
{
	"-f",
	"-d",
	"-O",
	"@FROM@",
	"@TO@",
	NULL
};
const char* DIFFTOOL__GVIMDIFF__ARGS__LINUX[] =
{
	"-f",
	"-d",
	"-g",
	"-O",
	"@FROM@",
	"@TO@",
	NULL
};
#define DIFFTOOL__GVIMDIFF__ARGS__MAC DIFFTOOL__GVIMDIFF__ARGS__LINUX

// Mergetool: Vim GUI (gvimdiff)
#define MERGETOOL__GVIMDIFF DIFFTOOL__GVIMDIFF
#define MERGETOOL__GVIMDIFF__PATHS__WINDOWS DIFFTOOL__GVIMDIFF__PATHS__WINDOWS
#define MERGETOOL__GVIMDIFF__PATHS__LINUX DIFFTOOL__GVIMDIFF__PATHS__LINUX
#define MERGETOOL__GVIMDIFF__PATHS__MAC DIFFTOOL__GVIMDIFF__PATHS__MAC
const char* MERGETOOL__GVIMDIFF__ARGS__WINDOWS[] =
{
	"-f",
	"-d",
	"-O",
	"@BASELINE@",
	"@RESULT@",
	"@OTHER@",
	NULL
};
const char* MERGETOOL__GVIMDIFF__ARGS__LINUX[] =
{
	"-f",
	"-d",
	"-g",
	"-O",
	"@BASELINE@",
	"@RESULT@",
	"@OTHER@",
	NULL
};
#define MERGETOOL__GVIMDIFF__ARGS__MAC MERGETOOL__GVIMDIFF__ARGS__LINUX
const char* MERGETOOL__GVIMDIFF__FLAGS[] =
{
	SG_MERGETOOL__FLAG__ANCESTOR_OVERWRITE,
	NULL
};
#define MERGETOOL__GVIMDIFF__FLAGS__WINDOWS MERGETOOL__GVIMDIFF__FLAGS
#define MERGETOOL__GVIMDIFF__FLAGS__LINUX MERGETOOL__GVIMDIFF__FLAGS
#define MERGETOOL__GVIMDIFF__FLAGS__MAC MERGETOOL__GVIMDIFF__FLAGS
