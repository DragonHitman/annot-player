#ifndef _QTEXt_ACTIONWITHID_H
#define _QTEXt_ACTIONWITHID_H

// qtext/actionwithid.h
// 9/2/2011

#include <QAction>

namespace QtExt {

class ActionWithId : public QAction
{
  Q_OBJECT
  typedef ActionWithId Self;
  typedef QAction Base;

  int id_; // init to be zero

public:
  ActionWithId(QObject *parent);
  ActionWithId(const QString &text, QObject *parent);
  ActionWithId(const QIcon &icon, const QString &text, QObject *parent);

  ActionWithId(int id, QObject *parent);
  ActionWithId(int id, const QString &text, QObject *parent);
  ActionWithId(int id, const QIcon &icon, const QString &text, QObject *parent);

  int id() const { return id_; }
public slots:
  void setId(int id) { id_ = id; }

signals:
  void changedWithId(int id);
  void hoveredWithId(int id);
  void toggledWithId(int id, bool checked);
  void triggeredWithId(int id, bool checked = false);

// - Implementations -
private slots:
  void emit_changedWithId() { emit changedWithId(id_); }
  void emit_hoveredWithId() { emit hoveredWithId(id_); }
  void emit_toggledWithId(bool checked) { emit toggledWithId(id_, checked); }
  void emit_triggeredWithId(bool checked = false) { emit triggeredWithId(id_, checked); }

private:
  void createConnections();
};

} // namespace QtExt

#endif // _QTEXT_ACTIONWITHID_H