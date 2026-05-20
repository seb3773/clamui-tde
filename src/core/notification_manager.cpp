/*
 * notification_manager.cpp - System and Internal Notifications
 */

#include "notification_manager.h"
#include "settings_manager.h"
#include "../embedded_icons.h"

#include <tqapplication.h>
#include <tqdialog.h>
#include <tqlabel.h>
#include <tqpushbutton.h>
#include <tqvbox.h>
#include <tqlayout.h>
#include <tqhbox.h>
#include <tdelocale.h>
#include <tqdesktopwidget.h>
#include <tqtimer.h>

#include <dlfcn.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

NotificationManager* NotificationManager::s_instance = 0;

typedef void NotifyNotification_tqt;
typedef NotifyNotification_tqt* (*p_notify_notification_new_tqt)(const char* summary, const char* body, const char* icon);
typedef int (*p_notify_init_tqt)(const char* app_name);
typedef void (*p_notify_uninit_tqt)(void);
typedef int (*p_notify_notification_show_tqt)(NotifyNotification_tqt* n, void* error);
typedef void (*p_notify_notification_set_timeout_tqt)(NotifyNotification_tqt* n, int timeout_ms);
typedef void (*p_g_object_unref_tqt)(void* obj);

static void* tqt_libnotify_handle = 0;
static void* tqt_libgobject_handle = 0;
static p_notify_init_tqt tqt_notify_init = 0;
static p_notify_uninit_tqt tqt_notify_uninit = 0;
static p_notify_notification_new_tqt tqt_notify_notification_new = 0;
static p_notify_notification_show_tqt tqt_notify_notification_show = 0;
static p_notify_notification_set_timeout_tqt tqt_notify_notification_set_timeout = 0;
static p_g_object_unref_tqt tqt_g_object_unref = 0;
static int tqt_libnotify_inited = 0;

NotificationManager::NotificationManager(TQObject *parent)
    : TQObject(parent)
{
    s_instance = this;
}

NotificationManager::~NotificationManager()
{
    if (s_instance == this)
        s_instance = 0;
}

NotificationManager* NotificationManager::instance()
{
    if (!s_instance)
        new NotificationManager(tqApp);
    return s_instance;
}

void NotificationManager::sendNotification(const TQString &title, const TQString &message)
{
    SettingsManager settings;
    bool useDaemon = settings.valueBool("notify_daemon", false);
    bool useInternal = settings.valueBool("notify_internal", true);

    if (useDaemon) {
        sendLibnotify(title, message);
    }
    
    if (useInternal) {
        sendInternalPopup(title, message);
    }
}

bool NotificationManager::tqt_try_init_libnotify()
{
    if (tqt_libnotify_inited)
        return (tqt_notify_init && tqt_notify_notification_new && tqt_notify_notification_show && tqt_g_object_unref) ? 1 : 0;
    tqt_libnotify_inited = 1;

    const char* cands[] = {
        "libnotify.so.4",
        "libnotify.so.5",
        "libnotify.so.0",
        "libnotify.so",
        0
    };
    for (int i = 0; cands[i]; i++) {
        tqt_libnotify_handle = dlopen(cands[i], RTLD_LAZY | RTLD_LOCAL);
        if (tqt_libnotify_handle)
            break;
    }
    if (!tqt_libnotify_handle)
        return 0;

    tqt_notify_init = (p_notify_init_tqt)dlsym(tqt_libnotify_handle, "notify_init");
    tqt_notify_uninit = (p_notify_uninit_tqt)dlsym(tqt_libnotify_handle, "notify_uninit");
    tqt_notify_notification_new = (p_notify_notification_new_tqt)dlsym(tqt_libnotify_handle, "notify_notification_new");
    tqt_notify_notification_show = (p_notify_notification_show_tqt)dlsym(tqt_libnotify_handle, "notify_notification_show");
    tqt_notify_notification_set_timeout = (p_notify_notification_set_timeout_tqt)dlsym(tqt_libnotify_handle, "notify_notification_set_timeout");

    tqt_g_object_unref = (p_g_object_unref_tqt)dlsym(RTLD_DEFAULT, "g_object_unref");
    if (!tqt_g_object_unref) {
        const char* gcands[] = {
            "libgobject-2.0.so.0",
            "libgobject-2.0.so",
            0
        };
        for (int i = 0; gcands[i]; i++) {
            tqt_libgobject_handle = dlopen(gcands[i], RTLD_LAZY | RTLD_LOCAL);
            if (tqt_libgobject_handle)
                break;
        }
        if (tqt_libgobject_handle)
            tqt_g_object_unref = (p_g_object_unref_tqt)dlsym(tqt_libgobject_handle, "g_object_unref");
    }

    if (!tqt_notify_init || !tqt_notify_notification_new || !tqt_notify_notification_show || !tqt_g_object_unref)
        return 0;

    (void)tqt_notify_init("tdeClamUI");
    return 1;
}

const char* NotificationManager::ensure_notify_icon_tmp()
{
    static const char* kPath = "/tmp/tdeclamui_icon.png";
    static int inited = 0;
    if (inited)
        return kPath;
    inited = 1;

    if (access(kPath, R_OK) == 0)
        return kPath;

    int fd = open(kPath, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0)
        return 0;

    const unsigned char* data = main_icon_png;
    const int len = (int)main_icon_png_len;
    ssize_t wr = write(fd, (const void*)data, (size_t)len);
    (void)wr;
    close(fd);
    return kPath;
}

void NotificationManager::sendLibnotify(const TQString &summary, const TQString &body)
{
    if (!tqt_try_init_libnotify())
        return;

    const char* iconPath = ensure_notify_icon_tmp();
    if (!iconPath)
        iconPath = 0;
        
    const TQCString s = summary.utf8();
    const TQCString b = body.utf8();

    NotifyNotification_tqt* n = tqt_notify_notification_new(s.data(), b.data(), iconPath);
    if (!n)
        return;
    if (tqt_notify_notification_set_timeout)
        tqt_notify_notification_set_timeout(n, 5000);
    (void)tqt_notify_notification_show(n, 0);
    tqt_g_object_unref(n);
}

class InternalPopup : public TQDialog {
public:
    InternalPopup(TQWidget *parent, const TQString &title, const TQString &msg)
        : TQDialog(parent, 0, false, TQt::WType_Popup | TQt::WStyle_Customize | TQt::WStyle_NoBorder | TQt::WStyle_StaysOnTop | TQt::WDestructiveClose)
    {
        TQVBoxLayout *mainLayout = new TQVBoxLayout(this, 0, 0);
        TQFrame *frame = new TQFrame(this);
        frame->setFrameStyle(TQFrame::Box | TQFrame::Raised);
        frame->setLineWidth(2);
        frame->setPaletteBackgroundColor(TQColor(50, 50, 50));
        mainLayout->addWidget(frame);
        
        TQVBoxLayout *layout = new TQVBoxLayout(frame, 10, 5);
        
        TQHBoxLayout *topLayout = new TQHBoxLayout(layout, 0);
        TQLabel *lblTitle = new TQLabel(title, frame);
        TQFont f = lblTitle->font();
        f.setBold(true);
        lblTitle->setFont(f);
        lblTitle->setPaletteForegroundColor(TQColor(255, 255, 255));
        topLayout->addWidget(lblTitle);
        topLayout->addStretch(1);
        
        TQLabel *lblMsg = new TQLabel(msg, frame);
        lblMsg->setPaletteForegroundColor(TQColor(200, 200, 200));
        layout->addWidget(lblMsg);
        
        TQHBoxLayout *btnLayout = new TQHBoxLayout(layout, 0);
        btnLayout->addStretch(1);
        TQPushButton *btnClose = new TQPushButton(i18n("Close"), frame);
        connect(btnClose, TQT_SIGNAL(clicked()), this, TQT_SLOT(accept()));
        btnLayout->addWidget(btnClose);
        
        // Auto close after 10s
        TQTimer::singleShot(10000, this, TQT_SLOT(accept()));
    }
    
    void showBottomRight() {
        adjustSize();
        TQRect desk = TQApplication::desktop()->availableGeometry(TQApplication::desktop()->primaryScreen());
        move(desk.right() - width() - 20, desk.bottom() - height() - 20);
        show();
    }
};

void NotificationManager::sendInternalPopup(const TQString &summary, const TQString &body)
{
    InternalPopup *popup = new InternalPopup(0, summary, body);
    popup->showBottomRight();
}

#include "notification_manager.moc"
