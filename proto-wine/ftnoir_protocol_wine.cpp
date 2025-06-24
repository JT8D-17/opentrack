#include "ftnoir_protocol_wine.h"
#ifndef OTR_WINE_NO_WRAPPER
#   include "csv/csv.h"
#endif
#include "compat/library-path.hpp"

#include <cstring>
#include <cmath>

#include <QString>
#include <QDebug>

#include <QDir>
#include <QFileInfo>

wine::wine() = default;

wine::~wine()
{
#ifndef OTR_WINE_NO_WRAPPER
    bool exit = false;
    if (shm) {
        shm->stop = true;
        exit = wrapper.waitForFinished(100);
        if (exit)
            qDebug() << "proto/wine: wrapper exit code" << wrapper.exitCode();
    }
    if (!exit)
    {
        if (wrapper.state() != QProcess::NotRunning)
            wrapper.kill();
        wrapper.waitForFinished(1000);
    }
#endif
    //shm_unlink("/" WINE_SHM_NAME);
}

void wine::pose(const double *headpose, const double*)
{
    if (shm)
    {
        lck_shm.lock();
        for (int i = 3; i < 6; i++)
            shm->data[i] = (headpose[i] * M_PI) / 180;
        for (int i = 0; i < 3; i++)
            shm->data[i] = headpose[i] * 10;
#ifndef OTR_WINE_NO_WRAPPER
        if (shm->gameid != gameid)
        {
            QString gamename;
            QMutexLocker foo(&game_name_mutex);
            /* only EZCA for FSX requires dummy process, and FSX doesn't work on Linux */
            /* memory-hacks DLL can't be loaded into a Linux process, either */
            getGameData(shm->gameid, shm->table, gamename);
            gameid = shm->gameid2 = shm->gameid;
            connected_game = gamename;
        }
#endif
        lck_shm.unlock();
    }
}

module_status wine::initialize()
{
#ifndef OTR_WINE_NO_WRAPPER
    static const QString library_path(OPENTRACK_BASE_PATH + OPENTRACK_LIBRARY_PATH);

    auto env = QProcessEnvironment::systemEnvironment();

    QString wine_path = "None";
    QString wine_prefix = "None";

    if (s.variant_wine) {
        /* Add Wineprefix */
        if (!s.wineprefix->isEmpty())
            wine_prefix = s.wineprefix;
        if (wine_prefix[0] == '~')
            wine_prefix = qgetenv("HOME") + wine_prefix.mid(1);
        if (wine_prefix[0] != '/')
            return error(tr("Wine/Proton prefix must be an absolute path (given '%1')").arg(wine_prefix));
        /* Add WINE runtime */
        if (!s.wineruntime->isEmpty())
            wine_path = s.wineruntime;
        if (wine_path[0] == '~')
            wine_path = qgetenv("HOME") + wine_path.mid(1);
        if (wine_path[0] != '/')
            return error(tr("Wine/Proton runtime must be an absolute path (given '%1')").arg(wine_path));
        if(!QFileInfo(wine_path + "/bin/wine").exists())
            return error(tr("No WINE binary found at '%1'. Please pick another folder.").arg(wine_path));
        if(QDir(wine_path + "/bin/wine").exists())
            return error(tr("Target '%1' is a folder, not a binary. Please pick another folder.").arg(wine_path + "/bin/wine"));
        /* Determine if Proton or WINE */
        if (wine_path.contains("Proton", Qt::CaseInsensitive)) {    // returns true
            qDebug() << "Proton folder detected at " << wine_path;
        } else {
            qDebug() << "Regular WINE folder detected at " << wine_path;
            env.insert("WINEDLLPATH", wine_path+"/lib:"+wine_path+"/lib64:"+wine_path+"/lib32");
            env.insert("LD_LIBRARY_PATH", wine_path+"/lib:"+wine_path+"/lib64:"+wine_path+"/lib32");
        }
    }

    if (s.variant_proton) {
        if (s.proton_appid == "None")
            return error(tr("No app ID specified for Proton (Steam Play), please select one from the configuration dialog."));
        /* Look for Steam libraries on the system - see proton_handling.cpp */
        QVector<QString> steam_libs;
        get_steam_libs(steam_libs);
        /* Look for Proton runtime version and Proton prefix for the Steam app in the Steam libraries */
        for (int i = 0; i < steam_libs.size(); ++i) {
            QString searchpath;
            QVector<QString> returnvector;
            adjust_path(steam_libs.at(i),"/steamapps/compatdata/",searchpath); /* Adjust search path for home folder path - see proton_handling.cpp */
            get_proton_paths(searchpath,s.proton_appid,steam_libs,returnvector); /* Gets app's Proton runtime and prefix paths - see proton_handling.cpp */
            if (returnvector.size() == 2) {
                wine_path = returnvector[0]; /* App's Proton runtime path (/dist/ folder!) */
                wine_prefix = returnvector[1]; /* App's Proton prefix path */
            }
        }

        QString ld_library_paths = wine_path+"/lib64:"+wine_path+"/lib:";
        QString steamruntimefolder = QDir::homePath() + "/.local/share/Steam/ubuntu12_32/steam-runtime";
        ld_library_paths = ld_library_paths
        +"/overrides/lib/x86_64-linux-gnu:"
        +"/overrides/lib/x86_64-linux-gnu/aliases:"
        +"/overrides/lib/i386-linux-gnu:"
        +"/overrides/lib/i386-linux-gnu/aliases:"
        +steamruntimefolder+"/pinned_libs_32:"
        +steamruntimefolder+"/pinned_libs_64:"
        +steamruntimefolder+"/i386/lib/i386-linux-gnu:"
        +steamruntimefolder+"/i386/lib:"
        +steamruntimefolder+"/i386/usr/lib/i386-linux-gnu:"
        +steamruntimefolder+"/i386/usr/lib:"
        +steamruntimefolder+"/amd64/lib/x86_64-linux-gnu:"
        +steamruntimefolder+"/amd64/lib:"
        +steamruntimefolder+"/amd64/usr/lib/x86_64-linux-gnu:"
        +steamruntimefolder+"/amd64/usr/lib";

        /* LD_LIBRARY_PATH=": /media/Data/SteamLibrary/steamapps/common/Elite Dangerous" */

        env.insert("LD_LIBRARY_PATH", ld_library_paths);
        env.insert("WINEDLLPATH",wine_path+"/lib64/wine:"+wine_path+"/lib/wine");
        env.insert("WINEDEBUG","-all");
        env.insert("WINE_LARGE_ADDRESS_AWARE", "1");
    }
    /* Check and set path to WINE/Proton prefix */
    if (wine_prefix == "None") {
        return error(tr("No WINE/Proton prefix path set. Please Opentrack's settings."));
    } else {
        env.insert("WINEPREFIX", wine_prefix);
    }
    /* Check and set path to WINE binary */
    if (wine_path == "None") {
        return error(tr("No WINE/Proton runtime path set. Please Opentrack's settings."));
    } else {
        env.insert("PATH",wine_path+"/bin/:/usr/bin:/bin");
        env.insert("WINELOADER", wine_path + "/bin/wine");
        /* env.insert("WINEPATH", wine_path + "/bin");
        env.insert("WINESERVER", wine_path + "/bin/wineserver"); */
    }

    if (s.esync)
        env.insert("WINEESYNC", "1");
    if (s.fsync)
        env.insert("WINEFSYNC", "1");

    env.insert("OTR_WINE_PROTO", QString::number(s.protocol+1));

    /* Last stop for debugging */
    qDebug() << "LD_LIBRARY_PATH: " << env.value("LD_LIBRARY_PATH");
    qDebug() << "WINEDLLPATH: " << env.value("WINEDLLPATH");
    qDebug() << "WINEDEBUG: " << env.value("WINEDEBUG");
    qDebug() << "WINE_LARGE_ADDRESS_AWARE: " << env.value("WINE_LARGE_ADDRESS_AWARE");
    qDebug() << "WINEPREFIX: " << env.value("WINEPREFIX");
    qDebug() << "PATH: " << env.value("PATH");
    qDebug() << "WINELOADER: "<< env.value("WINELOADER");
    /* qDebug() << "WINEPATH: " << env.value("WINEPATH");
    qDebug() << "WINESERVER: " << env.value("WINESERVER"); */
    qDebug() << "WINEESYNC: " << env.value("WINEESYNC");
    qDebug() << "WINEFSYNC: " << env.value("WINEFSYNC");



    /* Prepare and start wrapper */
    QString run_wine_path = wine_path + "/bin/wine";
    /* qDebug() << "Launching WINE server from" << run_wine_path; */
    /* qDebug() << library_path; */
    wrapper.setProcessEnvironment(env);
    wrapper.setWorkingDirectory(OPENTRACK_BASE_PATH);
    wrapper.start(run_wine_path, { library_path + s.wrapper });
#endif

    if (lck_shm.success())
    {
        shm = (WineSHM*) lck_shm.ptr();
        memset(shm, 0, sizeof(*shm));
    }

    if (lck_shm.success())
        return status_ok();
    else
        return error(tr("Can't open shared memory mapping"));
}

OPENTRACK_DECLARE_PROTOCOL(wine, FTControls, wine_metadata)
