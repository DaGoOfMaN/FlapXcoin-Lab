#ifndef NETWORKPAGE_H
#define NETWORKPAGE_H

#include <QWidget>
#include "clientmodel.h"
#include "main.h"
#include "wallet.h"
#include "base58.h"

#include <QDir>
#include <QFile>
#include <QProcess>
#include <QTime>
#include <QTimer>
#include <QStringList>
#include <QMap>
#include <QSettings>
#include <QSlider>

QT_BEGIN_NAMESPACE
class QModelIndex;

QT_END_NAMESPACE

namespace Ui {
    class NetworkPage;
}
class ClientModel;
class WalletModel;
class TxViewDelegate;
class TransactionFilterProxy;
class QLabel;
class QMenu;
class QFrame;
class QHBoxLayout;

/** Overview ("home") page widget */
class NetworkPage : public QWidget
{
    Q_OBJECT
public:
    explicit NetworkPage(QWidget *parent = 0);
    ~NetworkPage();

    void setClientModel(ClientModel *clientModel);
    void setWalletModel(WalletModel *walletModel);
    void showOutOfSyncWarning(bool fShow);

    int heightPrevious;
    int connectionPrevious;
    int volumePrevious;
    int stakeminPrevious;
    int stakemaxPrevious;
    QString stakecPrevious;
    double rewardPrevious;
    double netPawratePrevious;
    QString pawratePrevious;
    double hardnessPrevious;
    double hardnessPrevious2;

public slots:
    void setStatistics(ClientModel *modelStatistics);
    void updateStatistics();
    void updatePrevious(int, int, int, double, double, double, double, QString, int, int);

signals:
    void transactionClicked(const QModelIndex &index);

private:
    Ui::NetworkPage *ui;
    ClientModel *clientModel;
    WalletModel *walletModel;
    CWallet *wallet;
    ClientModel *modelStatistics;
    qint64 currentBalance;
    qint64 currentStake;
    qint64 interest;
    qint64 currentUnconfirmedBalance;
    qint64 currentImmatureBalance;
    QMenu *contextMenu;

    //Weight label
    qint64 currentWeight;
    qint64 currentNetworkWeight;
    qint64 currentMyWeight;

    TxViewDelegate *txdelegate;
    TransactionFilterProxy *filter;

private slots:
    void handleTransactionClicked(const QModelIndex &index);
    void updateMyWeight();

signals:
    void stakeForCharitySignal();
};

#endif // NETWORKPAGE_H
