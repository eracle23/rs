// Minimal shell-level theme synchronizer: listen for palette/style changes
// and (re)apply lightweight brand QSS that respects QPalette roles.

#ifndef __ThemeSync_h
#define __ThemeSync_h

#include <QObject>

class ThemeSync : public QObject
{
  Q_OBJECT
public:
  explicit ThemeSync(QObject* parent = nullptr);
  ~ThemeSync() override = default;

  bool eventFilter(QObject* watched, QEvent* event) override;

Q_SIGNALS:
  /// 每次品牌样式应用完成后发出，供 SliceColorAdapter 等订阅者同步 MRML 颜色
  void brandingApplied();

public Q_SLOTS:
  void applyBranding();

private:
  bool pending_{false};
  bool applying_{false};
};

#endif
