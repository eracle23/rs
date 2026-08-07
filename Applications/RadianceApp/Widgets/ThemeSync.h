// Minimal shell-level theme synchronizer: listen for palette/style changes
// and (re)apply lightweight brand QSS that respects QPalette roles.

#ifndef __ThemeSync_h
#define __ThemeSync_h

#include <QObject>
#include <QHash>
#include <QSet>
#include <QColor>
#include <QString>

class ThemeSync : public QObject
{
  Q_OBJECT
public:
  explicit ThemeSync(QObject* parent = nullptr);
  ~ThemeSync() override = default;

  bool eventFilter(QObject* watched, QEvent* event) override;

public Q_SLOTS:
  void applyBranding();

private:
  void refreshSliceControllers();
  bool pending_{false};
  bool applying_{false};
  QHash<QString, QColor> tunedSliceNodeColors_;
  QSet<QString> userOverriddenSliceNodes_;
};

#endif
