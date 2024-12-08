#include "ftnoir_protocol_wine.h"
#include <QDebug>
#include <QDir>
#include <QVector>

#include "api/plugin-api.hpp"

QVector< QVector<QString> > steamapps;
QVector<QString> steam_libraries;

FTControls::FTControls()
{
    ui.setupUi(this);
    /* Clear all items from Vectors */
    steamapps.clear();
    steam_libraries.clear();
    /* Get all Steam libraries on the system - see proton_handling.cpp */
    get_steam_libs(steam_libraries);
    /*
     * Collect Steam app information. ID, name, path to Proton runtime, path to Proton prefix
     */
    for (int i = 0; i < steam_libraries.size(); ++i) {
        QString searchpath;
        adjust_path(steam_libraries.at(i),"/steamapps",searchpath); /* Adjust search path for home folder path - see proton_handling.cpp */
        /* Look for appmanifest files and iterate through them */
        QDir dir(searchpath);
        dir.setFilter(QDir::Files);
        dir.setNameFilters({ "appmanifest_*" });
        QFileInfoList list = dir.entryInfoList();
        for (int j = 0; j < list.size(); ++j) {
            QFileInfo fileInfo = list.at(j);
            QFile manifestfile(fileInfo.filePath());
            /* Extract Steam app ID and name from manifest file */
            if (manifestfile.open(QIODevice::ReadOnly | QIODevice::Text)) {
                const QRegularExpression nameExp1("\"appid\"\\s+\"(.*)\""); /* steamapps[0]: Steam app ID */
                const QRegularExpression nameExp2("\"name\"\\s+\"(.*)\""); /* steamapps[1]: Steam app name */
                QTextStream in(&manifestfile);
                QString line;
                QVector<QString> tempvec;
                while (in.readLineInto(&line)) {
                    const auto nameMatch1 = nameExp1.match(line);
                    const auto nameMatch2 = nameExp2.match(line);
                    if(nameMatch1.hasMatch() && tempvec.size() == 0) {
                        tempvec.push_back(nameMatch1.captured(1));
                    }
                    if(nameMatch2.hasMatch() && tempvec.size() == 1) {
                        tempvec.push_back(nameMatch2.captured(1));
                    }
                }
                if (tempvec.size() == 2) {
                    steamapps.push_back(tempvec);       /* Now contains Steam app ID and name */
                    /* qDebug() << steamapps[steamapps.size() - 1]; */
                }
            }
        }
        /* Get proton runtime and path plus prefix path for the Steam app */
        adjust_path(steam_libraries.at(i),"/steamapps/compatdata/",searchpath); /* Adjust search path for home folder path - see proton_handling.cpp */
        for (int j = 0; j < steamapps.size(); ++j) {
            QVector<QString> returnvector;
            get_proton_paths(searchpath,steamapps[j][0],steam_libraries,returnvector); /* Gets app's Proton runtime and prefix paths - see proton_handling.cpp */
            if (returnvector.size() == 2 and steamapps[j].size() == 2) {
                steamapps[j].push_back(returnvector[0]); /* steamapps[2]: App's Proton runtime path */
                steamapps[j].push_back(returnvector[1]); /* steamapps[3]: App's Proton prefix path */
            }
        }
    }
    /*
     * Clean Steamapps vector from elements that do not have their Proton information fields filled (size < 3)
     */
    for(int i = steamapps.size() - 1; i >= 0; i--) {
        if (steamapps[i].size() != 4) {
            /* qDebug() << steamapps[i].size() << steamapps[i][1] << "does not use Proton. Deleting."; */
            steamapps.erase(steamapps.begin() + i );
        }
    }
    /*
     * Add Steam apps to UI combobox
     */
    for (int k = 0; k < steamapps.size(); ++k) {
        /* qDebug() << steamapps[k]; */
        ui.steamapp->addItem(steamapps[k][1]+" ("+steamapps[k][0]+")", QVariant{steamapps[k][0]});
        if (s.proton_appid == steamapps[k][0]) {
            /*qDebug() << "Index:" << k;*/
            ui.steamapp->setCurrentIndex(k);
        }
    }

    /*
     * WINE wrapper detection
     */
    QDir wrapperdir;
    wrapperdir.currentPath(); /* Starts in [opentrack install dir]/bin */
    wrapperdir.cdUp();
    wrapperdir.cd("libexec/opentrack/");
    if (wrapperdir.exists()) {
        wrapperdir.setFilter(QDir::Files);
        wrapperdir.setNameFilters({ "opentrack-wrapper-wine*.exe.so" });
        QFileInfoList wrapperlist = wrapperdir.entryInfoList();
        for(int i=0;i<wrapperlist.size();++i) {
            QFileInfo fileInfo = wrapperlist.at(i);
            QString wrapperfile = fileInfo.fileName();
            ui.wrapper_selection->addItem(wrapperfile, QVariant{wrapperfile});
            if(s.wrapper == wrapperfile)
                ui.wrapper_selection->setCurrentIndex(i);
        }
    } else {
     qDebug() << "\"[opentrack install directory]/libexec/opentrack\" not found";
    }
    tie_setting(s.wineprefix, ui.wineprefix);
    tie_setting(s.wineruntime, ui.wineruntime);
    tie_setting(s.variant_wine, ui.variant_wine);
    tie_setting(s.variant_proton, ui.variant_proton);
    tie_setting(s.esync, ui.esync);
    tie_setting(s.fsync, ui.fsync);
    tie_setting(s.protocol, ui.protocol_selection);
    tie_setting(s.wrapper, ui.wrapper_selection);

    connect(ui.buttonBox, &QDialogButtonBox::accepted, this, &FTControls::doOK);
    connect(ui.buttonBox, &QDialogButtonBox::rejected, this, &FTControls::doCancel);
}

void FTControls::doOK()
{
    if (ui.variant_wine) {


    }
    /* Save Steam app id */
    if (ui.variant_proton) {
        s.proton_appid = ui.steamapp->itemData(ui.steamapp->currentIndex()).toString();
    }
    s.b->save();
    close();
}

void FTControls::doCancel()
{
    s.b->reload();
    close();
}
