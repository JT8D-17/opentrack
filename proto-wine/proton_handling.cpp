#include <QDebug>
#include <QDir>
#include <QVector>
#include <QString>
#include <QFile>
#include "ftnoir_protocol_wine.h"

/*
 * Adds standard Steam library locations to vector and collects other Steam libraries on the system
 */
void get_steam_libs(QVector<QString> & outvector)
{
    /* Define input stuff */
    QVector<QString> default_steamlibs({
    "~/.steam/root",
    "~/.steam/steam",
    "~/.local/share/Steam",
    });
    QString librarycontainer = QDir::homePath() + "/.steam/steam/steamapps/libraryfolders.vdf";

    /* Append outvector with default_steamlibs */
    outvector.append(default_steamlibs);
    /*
     * Scan for other Steam libraries than the default ones above and add the paths to the Steam libraries vector
     */
    QFile libraryfile(librarycontainer);
    if (libraryfile.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        const QRegularExpression nameExp("\"\\d+\"\\s+\"(.*)\"");
        QTextStream in(&libraryfile);
        QString line;
        while (in.readLineInto(&line)) {
            const auto nameMatch = nameExp.match(line);
            if(nameMatch.hasMatch())
            {
                /* qDebug() << "Found Steam library in libraryfolders.vdf at: " << nameMatch.captured(1); */
                outvector.append(nameMatch.captured(1));
            }
        }
    }
    /* for (int i = 0; i < outvector.size(); ++i)
        qDebug() << outvector[i]; */
}
/*
 *  Reads a property by regex from the input file and writes it to the output file
 */
void read_proton_property(QString inputpath,QString inregex,QString & outstring) {
    QFile inputfile(inputpath);
    if (inputfile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        const QRegularExpression regex(inregex);
        QTextStream in(&inputfile);
        QString line;
        while (in.readLineInto(&line)) {
            const auto nameMatch1 = regex.match(line);
            if(nameMatch1.hasMatch()) {
                outstring = nameMatch1.captured(1);
                /* qDebug() << inputfile << outstring; */
                break;
            }
        }
    }
}
/*
 * Analyzes the Proton version string composition in the version file and writes it to the output variable
 */
bool analyze_version_string(QString inputstring,QString inregex, QString & outstring) {
    const QRegularExpression regex(inregex);
    const auto stringmatch = regex.match(inputstring);
    if(stringmatch.hasMatch()) {
        outstring = stringmatch.captured(1);
        /* qDebug() << inputstring << inregex << outstring; */
        return true;
    } else {
        return false;
    }
}
/*
 * Gets the Proton version and the path to the corresponding /dist or /files folder
 */
void get_proton_version(QString inpath, QVector<QString> & inlibrary, QString & outpath)
{
    QVector<QString> protonversions({"None","None"});
    /* Get Proton version from version file - Do not change output variable! */
    read_proton_property(inpath + "/version","(\\d.+)",protonversions[0]);
    /* Get Proton version from config_info file */
    read_proton_property(inpath + "/config_info","(\\d.+)",protonversions[1]);
    /* Information from version file and config_info plus matching Proton versions: Get proton runtime path from config_info */
    if (protonversions[0] != "None" && protonversions[1] != "None" && protonversions[0] == protonversions[1]) {
            read_proton_property(inpath + "/config_info","(.*/dist)",outpath);
            if (outpath == "")
                read_proton_property(inpath + "/config_info","(.*/files)",outpath); /* Work around Proton Experimental's path */
            /* qDebug() << inpath << outpath; */
    }
    /* Information from only one of the two files (version file or config_info) or non-matching Proton versions: Get proton runtime path from version file and a search through library folders*/
    if (protonversions[0] != protonversions[1]) {
        QString versionstring;
        QString cleanfoldername;
        /* Regex-search path for any sign of GloriousEggroll Proton releases (folder name usually Proton-x.x-GE-somethingsomething */
        for (int j=0; j<protonversions.size();++j) {
            if (protonversions[j] != "None") {
                if (analyze_version_string(protonversions[j],"(\\d+.\\d+-\\D+.*)",versionstring)) {
                    cleanfoldername = "Proton-" + versionstring;
                } else if (analyze_version_string(protonversions[j],"(\\d+.\\d+)-\\d+",versionstring)) {
                    cleanfoldername = "Proton " + versionstring;
                }
            }
        }

        for (int l = 0; l < inlibrary.size(); ++l) {
            QString subsearchpath;
            adjust_path(inlibrary.at(l),"/steamapps/common/"+cleanfoldername,subsearchpath);
            /* if (l < 2) {
                subsearchpath = QDir::homePath() + inlibrary.at(l) + "/steamapps/common/" + cleanfoldername;
            } else {
                subsearchpath = inlibrary.at(l) + "/steamapps/common/" + cleanfoldername;
            } */
            QFile versionfile(subsearchpath + "/version");
            if (versionfile.exists()) {
                outpath = subsearchpath + "/dist";
            }
        }
        /* Last chance to get a Proton runtime path from a version file only by looking in compatibilitytools.d */
        if (outpath == "") {
            QString tempfilepath = QDir::homePath()+"/.steam/root/compatibilitytools.d/"+cleanfoldername;
            QFile customprotonversionfile(tempfilepath+"/version");
            if (customprotonversionfile.exists()) {
                outpath = tempfilepath + "/dist";
            }
        }
    }
}
/*
 * Adjusts an input path for the location of the "/home/" folder
 */
void adjust_path(QString inpath, QString suffix, QString & outpath)
{
    const QRegularExpression pathbegin("^~/\\.");
    const auto pathmatch = pathbegin.match(inpath);
    if (pathmatch.hasMatch()) {
        outpath = QDir::homePath() + inpath.remove(QRegularExpression("~")) + suffix;
    } else {
        outpath = inpath + suffix;
    }
}
/*
 * Gets the Proton runtime's and Proton prefix' paths
 */
void get_proton_paths(QString input_path, QString input_appid, QVector<QString> & inlibrary, QVector<QString> & out_vector)
{
    QString configpath = input_path + input_appid;
    QString runtimepath;
    /* Gets the path to the Proton runtime for the Steam app */
    get_proton_version(configpath,inlibrary,runtimepath);
    if (runtimepath != "" and out_vector.size() == 0) {
        out_vector.push_back(runtimepath);
    }
    /* Gets the prefix path for the Steam app */
    QDir prefixpath(configpath + "/pfx");
    if (prefixpath.exists() and out_vector.size() == 1){
        out_vector.push_back(prefixpath.absolutePath());
    }
}
