#pragma once

#include "api/plugin-api.hpp"
#include "compat/shm.h"
#include "wine-shm.h"

#include "ui_ftnoir_winecontrols.h"

#include "options/options.hpp"
using namespace options;

#include <QMutex>
#include <QProcess>
#include <QString>
#include <QVariant>

#include <QDebug>
#include <QVector>

/* Function declaration for Steam/Proton related things */
void get_steam_libs(QVector<QString> & outvector);
bool analyze_version_string(QString inputstring,QString inregex, QString & outstring);
void get_proton_version(QString inpath,QVector<QString> & inlibrary, QString & outpath);
void read_proton_property(QString inputpath,QString inregex,QString & outstring);
void adjust_path(QString inpath, QString suffix, QString & outpath);
void get_proton_paths(QString input_path, QString input_appid, QVector<QString> & inlibrary, QVector<QString> & out_vector);

struct settings : opts
{
    settings() : opts{"proto-wine"} {}
    value<bool> variant_proton{b, "variant-proton", false },
                variant_wine{b, "variant-wine", true },
                fsync{b, "fsync", true},
                esync{b, "esync", true};
    value<int>     protocol{b, "protocol", 2};

    value<QString>  wineprefix{b, "wineprefix", "~/.wine"},
                    wineruntime{b, "wineruntime", "/usr"},
                    proton_appid{b, "proton_appid", "None"},
                    wrapper{b, "wrapper", "opentrack-wrapper-wine.exe.so"};
};

class wine : TR, public IProtocol
{
    Q_OBJECT

public:
    wine();
    ~wine() override;

    module_status initialize() override;
    void pose(const double* headpose, const double*) override;

    QString game_name() override
    {
#ifndef OTR_WINE_NO_WRAPPER
        QMutexLocker foo(&game_name_mutex);
        return connected_game;
#else
        return QStringLiteral("X-Plane");
#endif
    }
private:
    shm_wrapper lck_shm { WINE_SHM_NAME, WINE_MTX_NAME, sizeof(WineSHM) };
    WineSHM* shm = nullptr;
    settings s;

#ifndef OTR_WINE_NO_WRAPPER
    QProcess wrapper;
    int gameid = 0;
    QString connected_game;
    QMutex game_name_mutex;
#endif
};

class FTControls: public IProtocolDialog
{
    Q_OBJECT

public:
    FTControls();
    void register_protocol(IProtocol *) override {}
    void unregister_protocol() override {}

private:
    Ui::UICFTControls ui;
    settings s;

private slots:
    void doOK();
    void doCancel();
};

class wine_metadata : public Metadata
{
    Q_OBJECT

public:
#ifndef OTR_WINE_NO_WRAPPER
    QString name() override { return tr("Wine -- Windows layer for Unix"); }
    QIcon icon() override { return QIcon(":/images/wine.png"); }
#else
    QString name() override { return tr("X-Plane"); }
    QIcon icon() override { return {}; }
#endif
};
