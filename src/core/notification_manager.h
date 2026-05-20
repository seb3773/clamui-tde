/*
 * notification_manager.h - System and Internal Notifications
 */

#ifndef CLAM_NOTIFICATION_MANAGER_H
#define CLAM_NOTIFICATION_MANAGER_H

#include <tqobject.h>
#include <tqstring.h>

class NotificationManager : public TQObject
{
    TQ_OBJECT

public:
    NotificationManager(TQObject *parent = 0);
    ~NotificationManager();

    static NotificationManager* instance();

    void sendNotification(const TQString &title, const TQString &message);

private:
    static NotificationManager *s_instance;
    bool tqt_try_init_libnotify();
    void sendLibnotify(const TQString &summary, const TQString &body);
    void sendInternalPopup(const TQString &summary, const TQString &body);
    const char* ensure_notify_icon_tmp();
};

#endif /* CLAM_NOTIFICATION_MANAGER_H */
