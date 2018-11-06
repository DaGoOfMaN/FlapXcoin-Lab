#include "networkpage.h"
#include "ui_networkpage.h"

#include "walletmodel.h"
#include "clientmodel.h"
#include "main.h"
#include "bitcoinunits.h"
#include "init.h"
#include "base58.h"
#include "bitcoingui.h"
#include "calcdialog.h"
#include "optionsmodel.h"
#include "transactiontablemodel.h"
#include "transactionfilterproxy.h"
#include "guiutil.h"
#include "guiconstants.h"
#include "wallet.h"
#include "bitcoinrpc.h"
#include "askpassphrasedialog.h"

#include <QAbstractItemDelegate>
#include <QPainter>
#include <QIcon>
#include <QWidget>
#include <QLabel>
#include <QTimer>
#include <QFrame>
#include <sstream>
#include <string>
#include <QMenu>


using namespace json_spirit;
extern CWallet* pwalletMain;
extern int64_t nLastCoinStakeSearchInterval;
double GetPoSKernelPS();


NetworkPage::NetworkPage(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::NetworkPage),
      clientModel(0),
      walletModel(0),
      currentWeight(0),
      currentNetworkWeight(0),


      filter(0)
  {
    ui->setupUi(this);

    if (GetBoolArg("-staking", true))
    {
        QTimer *timerMyWeight = new QTimer();
        connect(timerMyWeight, SIGNAL(timeout()), this, SLOT(updateMyWeight()));
        timerMyWeight->start(30 * 1000);
        updateMyWeight();
    }
}
void NetworkPage::updateMyWeight()
{
    uint64_t nMinWeight = 0, nMaxWeight = 0, nWeight = 0;
    if (nLastCoinStakeSearchInterval && pwalletMain && !IsInitialBlockDownload()) //flapx GetStakeWeight requires mutex lock on wallet which tends to freeze initial block downloads
        pwalletMain->GetStakeWeight(*pwalletMain, nMinWeight, nMaxWeight, nWeight);

    if (nLastCoinStakeSearchInterval && nWeight)
    {
        uint64_t nNetworkWeight = GetPoSKernelPS();
        unsigned nEstimateTime = nTargetSpacing * 2 * nNetworkWeight / nWeight;

        QString text;
        if (nEstimateTime < 60)
        {
            text = tr("%n second(s)", "", nEstimateTime);
        }
        else if (nEstimateTime < 60*60)
        {
            text = tr("%n minute(s)", "", nEstimateTime/60);
        }
        else if (nEstimateTime < 24*60*60)
        {
            text = tr("%n hour(s)", "", nEstimateTime/(60*60));
        }
        else
        {
            text = tr("%n day(s)", "", nEstimateTime/(60*60*24));
        }

        ui->labelMyWeight->setText(tr("Staking.<br>Your weight is %1<br>Network weight is %2<br>Expected time to earn reward is %3").arg(nWeight).arg(nNetworkWeight).arg(text));
    }
    else
    {
        if (pwalletMain && pwalletMain->IsLocked())
            ui->labelMyWeight->setText(tr("Not staking because your wallet is locked,<br> please unlock for staking."));
        else if (vNodes.empty())
            ui->labelMyWeight->setText(tr("Not staking because your wallet is offline,<br> please wait for a connection..."));
        else if (IsInitialBlockDownload())
            ui->labelMyWeight->setText(tr("Not staking because your wallet is syncing,<br> please wait for this process to end..."));
        else if (!nWeight)
            ui->labelMyWeight->setText(tr("Not staking because you don't have mature coins..."));
        else
            ui->labelMyWeight->setText(tr("Not staking"));
    }
}

void NetworkPage::updateStatistics()
{
    double pHardness = GetDifficulty();
    double pHardness2 = GetDifficulty(GetLastBlockIndex(pindexBest, true));
    int pPawrate = GetPoWMHashPS();
    double pPawrate2 = 0.000;
    int nHeight = pindexBest->nHeight;
    double nSubsidy = double (GetProofOfWorkReward(nHeight, 0, pindexBest->GetBlockHash())/ double (COIN));
    uint64_t nMinWeight = 0, nMaxWeight = 0, nWeight = 0;
    pwalletMain->GetStakeWeight(*pwalletMain, nMinWeight, nMaxWeight, nWeight);
    uint64_t nNetworkWeight = GetPoSKernelPS();
    int64_t volume = ((pindexBest->nMoneySupply)/100000000);
    int peers = this->modelStatistics->getNumConnections();
    pPawrate2 = (double)pPawrate;
    QString height = QString::number(nHeight);
    QString subsidy = QString::number(nSubsidy, 'f', 6);
    QString hardness = QString::number(pHardness, 'f', 6);
    QString hardness2 = QString::number(pHardness2, 'f', 6);
    QString pawrate = QString::number(pPawrate2, 'f', 3);
    QString Qlpawrate = modelStatistics->getLastBlockDate().toString();

    QString QPeers = QString::number(peers);
    QString qVolume = QLocale::system().toString((qlonglong)volume);

    if(nHeight > heightPrevious)
    {
        ui->heightBox->setText("<b><font color=\"green\">" + height + "</font></b>");
    }
    else
    {
        ui->heightBox->setText(height);
    }

    if(nSubsidy < rewardPrevious)
    {
        ui->rewardBox->setText("<b><font color=\"red\">" + subsidy + "</font></b>");
    }
    else
    {
        ui->rewardBox->setText(subsidy);
    }

    if(pHardness > hardnessPrevious)
    {
        ui->diffBox->setText("<b><font color=\"green\">" + hardness + "</font></b>");
    }
    else if(pHardness < hardnessPrevious)
    {
        ui->diffBox->setText("<b><font color=\"red\">" + hardness + "</font></b>");
    }
    else
    {
        ui->diffBox->setText(hardness);
    }

    if(pHardness2 > hardnessPrevious2)
    {
        ui->diffBox2->setText("<b><font color=\"green\">" + hardness2 + "</font></b>");
    }
    else if(pHardness2 < hardnessPrevious2)
    {
        ui->diffBox2->setText("<b><font color=\"red\">" + hardness2 + "</font></b>");
    }
    else
    {
        ui->diffBox->setText(hardness);
    }

    if(pPawrate2 > netPawratePrevious)
    {
        ui->pawrateBox->setText("<b><font color=\"green\">" + pawrate + " MH/s</font></b>");
    }
    else if(pPawrate2 < netPawratePrevious)
    {
        ui->pawrateBox->setText("<b><font color=\"red\">" + pawrate + " MH/s</font></b>");
    }
    else
    {
        ui->pawrateBox->setText(pawrate + " MH/s");
    }

    if(Qlpawrate != pawratePrevious)
    {
        ui->localBox->setText("<b><font color=\"green\">" + Qlpawrate + "</font></b>");
    }
    else
    {
    ui->localBox->setText(Qlpawrate);
    }

    if(peers > connectionPrevious)
    {
        ui->connectionBox->setText("<b><font color=\"green\">" + QPeers + "</font></b>");
    }
    else if(peers < connectionPrevious)
    {
        ui->connectionBox->setText("<b><font color=\"red\">" + QPeers + "</font></b>");
    }
    else
    {
        ui->connectionBox->setText(QPeers);
    }

    if(volume > volumePrevious)
    {
        ui->volumeBox->setText("<b>" + qVolume + " FLAPX" + "</font></b>");
    }
    else if(volume < volumePrevious)
    {
        ui->volumeBox->setText("<b>" + qVolume + " FLAPX" + "</font></b>");
    }
    else
    {
        ui->volumeBox->setText(qVolume + " FLAPX");
    }

    updatePrevious(nHeight, nMinWeight, nNetworkWeight, nSubsidy, pHardness, pHardness2, pPawrate2, Qlpawrate, peers, volume);
}

void NetworkPage::updatePrevious(int nHeight, int nMinWeight, int nNetworkWeight, double nSubsidy, double pHardness, double pHardness2, double pPawrate2, QString Qlpawrate, int peers, int volume)
{
    heightPrevious = nHeight;
    stakeminPrevious = nMinWeight;
    stakemaxPrevious = nNetworkWeight;
    rewardPrevious = nSubsidy;
    hardnessPrevious = pHardness;
    hardnessPrevious2 = pHardness2;
    netPawratePrevious = pPawrate2;
    pawratePrevious = Qlpawrate;
    connectionPrevious = peers;
    volumePrevious = volume;
}

void NetworkPage::setStatistics(ClientModel *modelStatistics)
{
    updateStatistics();
    this->modelStatistics = modelStatistics;

}

NetworkPage::~NetworkPage()
{
    delete ui;
}
