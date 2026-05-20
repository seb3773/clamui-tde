/*
 * profile_dialogs.h - Scan Profile Configuration Dialogs
 */

#ifndef CLAM_PROFILE_DIALOGS_H
#define CLAM_PROFILE_DIALOGS_H

#include <tqdialog.h>
#include "../core/profiles.h"

class TQLineEdit;
class TQTextEdit;
class TQCheckBox;

class ProfileEditorDialog : public TQDialog
{
    TQ_OBJECT

public:
    ProfileEditorDialog(TQWidget *parent, const ScanProfile &profile = ScanProfile());
    ~ProfileEditorDialog();
    
    ScanProfile getProfile() const;

private:
    TQLineEdit *m_nameEdit;
    TQLineEdit *m_descEdit;
    TQTextEdit *m_pathsEdit;
    TQTextEdit *m_exclusionsEdit;
    TQCheckBox *m_scanArchivesCb;
    TQCheckBox *m_heuristicCb;
    
    ScanProfile m_profile;
};

#endif /* CLAM_PROFILE_DIALOGS_H */
