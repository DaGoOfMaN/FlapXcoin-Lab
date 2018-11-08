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
#include "util.h"

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

#define DECORATION_SIZE 64
#define NUM_ITEMS 3

using namespace json_spirit;
extern CWallet* pwalletMain;
extern int64_t nLastCoinStakeSearchInterval;
double GetPoSKernelPS();
double GetPoSKernelPS2(const CBlockIndex* pindex);



NetworkPage::NetworkPage(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::NetworkPage),
    clientModel(0),
    walletModel(0),
    currentBalance(-1),
    currentStake(0),
    currentUnconfirmedBalance(-1),
    currentImmatureBalance(-1),
    currentWeight(0),
    currentNetworkWeight(0),


    filter(0)
{

    ui->setupUi(this);

    if(GetBoolArg("-chart", true))
    {
        // setup Plot
        // create graph
        ui->diffplot->addGraph();

        // give the axes some labels:
        ui->diffplot->xAxis->setLabel("Block Height");

        // set the pens
        ui->diffplot->graph(0)->setPen(QPen(QColor(40, 110, 173)));
        ui->diffplot->graph(0)->setLineStyle(QCPGraph::lsLine);

        // set axes label fonts:
        QFont label = font();
        ui->diffplot->xAxis->setLabelFont(label);
        ui->diffplot->yAxis->setLabelFont(label);
    }
    else
    {
        ui->diffplot->setVisible(false);
    }

    if (GetBoolArg("-staking", true))
    {
        QTimer *timerMyWeight = new QTimer();
        connect(timerMyWeight, SIGNAL(timeout()), this, SLOT(updateMyWeight()));
        timerMyWeight->start(30 * 1000);
        updateMyWeight();
    }

}



void NetworkPage::handleTransactionClicked(const QModelIndex &index)
{
    if(filter)
        emit transactionClicked(filter->mapToSource(index));
}


void NetworkPage::updateMyWeight()
{
    uint64_t nMinWeight = 0, nMaxWeight = 0, nWeight = 0;
    if (nLastCoinStakeSearchInterval && pwalletMain && !IsInitialBlockDownload()) //flapxcoin GetStakeWeight requires mutex lock on wallet which tends to freeze initial block downloads
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




void NetworkPage::setWalletModel(WalletModel *model)
{
    this->walletModel = model;
    if(model && model->getOptionsModel())
    {
        // Set up transaction list
        filter = new TransactionFilterProxy();
        filter->setSourceModel(model->getTransactionTableModel());
        filter->setLimit(NUM_ITEMS);
        filter->setDynamicSortFilter(true);
        filter->setSortRole(Qt::EditRole);
        filter->setShowInactive(false);
        filter->sort(TransactionTableModel::Date, Qt::DescendingOrder);

    }


    // update statistics
    updateStatistics();

    //set up a timer to auto refresh every 30 seconds to update the statistics
    QTimer *timerNetworkStats = new QTimer();
    connect(timerNetworkStats, SIGNAL(timeout()), this, SLOT(updateStatistics()));
    timerNetworkStats->start(30 * 1000);



}


// void NetworkPage::setModel(WalletModel *model)
void NetworkPage::setClientModel(ClientModel *model)
{
    // this->model = model;
    this->clientModel = model;


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
void NetworkPage::updatePlot(int count)
{
    static int64_t lastUpdate = 0;
    // Double Check to make sure we don't try to update the plot when it is disabled
    if(!GetBoolArg("-chart", true)) { return; }
    if (GetTime() - lastUpdate < 180) { return; } // This is just so it doesn't redraw rapidly during syncing

    if(fDebug) { printf("Plot: Getting Ready: pindexBest: %p\n", pindexBest); }

        bool fProofOfStake = (nBestHeight > 100000 + 100);
    if (fProofOfStake)
        ui->diffplot->yAxis->setLabel("24H Network Weight");
        else
        ui->diffplot->yAxis->setLabel("Difficulty");

    int numLookBack = 1440;
    double diffMax = 0;
    const CBlockIndex* pindex = GetLastBlockIndex(pindexBest, fProofOfStake);
    int height = pindex->nHeight;
    int xStart = std::max<int>(height-numLookBack, 0) + 1;
    int xEnd = height;

    // Start at the end and walk backwards
    int i = numLookBack-1;
    int x = xEnd;

    // This should be a noop if the size is already 2000
    vX.resize(numLookBack);
    vY.resize(numLookBack);

    if(fDebug) {
        if(height != pindex->nHeight) {
            printf("Plot: Warning: nBestHeight and pindexBest->nHeight don't match: %d:%d:\n", height, pindex->nHeight);
        }
    }

    if(fDebug) { printf("Plot: Reading blockchain\n"); }

    const CBlockIndex* itr = pindex;
    while(i >= 0 && itr != NULL)
    {
        if(fDebug) { printf("Plot: Processing block: %d - pprev: %p\n", itr->nHeight, itr->pprev); }
        vX[i] = itr->nHeight;
        if (itr->nHeight < xStart) {
            xStart = itr->nHeight;
        }
        vY[i] = fProofOfStake ? GetPoSKernelPS2(itr) : GetDifficulty(itr);
        diffMax = std::max<double>(diffMax, vY[i]);

        itr = GetLastBlockIndex(itr->pprev, fProofOfStake);
        i--;
        x--;
    }

    if(fDebug) { printf("Plot: Drawing plot\n"); }

    ui->diffplot->graph(0)->setData(vX, vY);

    // set axes ranges, so we see all data:
    ui->diffplot->xAxis->setRange((double)xStart, (double)xEnd);
    ui->diffplot->yAxis->setRange(0, diffMax+(diffMax/10));

    ui->diffplot->xAxis->setAutoSubTicks(false);
    ui->diffplot->yAxis->setAutoSubTicks(false);
    ui->diffplot->xAxis->setSubTickCount(0);
    ui->diffplot->yAxis->setSubTickCount(0);

    ui->diffplot->replot();

    if(fDebug) { printf("Plot: Done!\n"); }

    lastUpdate = GetTime();
}

