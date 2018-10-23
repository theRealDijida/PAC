// Copyright (c) 2014-2017 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "chainparams.h"
#include "dsnotificationinterface.h"
#include "instantx.h"
#include "governance.h"
#include "masternodeman.h"
#include "masternode-payments.h"
#include "masternode-sync.h"
#include "privatesend.h"
#ifdef ENABLE_WALLET
#include "privatesend-client.h"
#endif // ENABLE_WALLET
#include "txmempool.h"

void CDSNotificationInterface::InitializeCurrentBlockTip()
{
    LOCK(cs_main);
    UpdatedBlockTip(chainActive.Tip(), NULL, IsInitialBlockDownload());
}

void CDSNotificationInterface::AcceptedBlockHeader(const CBlockIndex *pindexNew)
{
    masternodeSync.AcceptedBlockHeader(pindexNew);
}

void CDSNotificationInterface::NotifyHeaderTip(const CBlockIndex *pindexNew, bool fInitialDownload)
{
    masternodeSync.NotifyHeaderTip(pindexNew, fInitialDownload, connman);
}

void CDSNotificationInterface::UpdatedBlockTip(const CBlockIndex *pindexNew, const CBlockIndex *pindexFork, bool fInitialDownload)
{
    if (pindexNew == pindexFork) // blocks were disconnected without any new ones
        return;

    masternodeSync.UpdatedBlockTip(pindexNew, fInitialDownload, connman);

    // Update global DIP0001 activation status
    fDIP0001ActiveAtTip = (VersionBitsState(pindexNew, Params().GetConsensus(), Consensus::DEPLOYMENT_DIP0001, versionbitscache) == THRESHOLD_ACTIVE);

    bool fNewFeeActiveAtTip = Params().GetConsensus().nFeeChangeBlock == pindexNew->nHeight;

    // Update min fees
    if (fNewFeeActiveAtTip) {
        if (!mapArgs.count("-minrelaytxfee")) {
            ::minRelayTxFee = CFeeRate(DEFAULT_MIN_RELAY_TX_FEE);
            mempool.UpdateMinFee(::minRelayTxFee);
        }
#ifdef ENABLE_WALLET
        if (!mapArgs.count("-mintxfee")) {
            CWallet::minTxFee = CFeeRate(DEFAULT_TRANSACTION_MINFEE);
        }
        if (!mapArgs.count("-fallbackfee")) {
            CWallet::fallbackFee = CFeeRate(DEFAULT_FALLBACK_FEE);
        }
#endif // ENABLE_WALLET
    }

    if (fInitialDownload)
        return;

    mnodeman.UpdatedBlockTip(pindexNew);
    CPrivateSend::UpdatedBlockTip(pindexNew);
#ifdef ENABLE_WALLET
    privateSendClient.UpdatedBlockTip(pindexNew);
#endif // ENABLE_WALLET
    instantsend.UpdatedBlockTip(pindexNew);
    mnpayments.UpdatedBlockTip(pindexNew, connman);
    governance.UpdatedBlockTip(pindexNew, connman);
}

void CDSNotificationInterface::SyncTransaction(const CTransaction &tx, const CBlock *pblock)
{
    instantsend.SyncTransaction(tx, pblock);
    CPrivateSend::SyncTransaction(tx, pblock);
}
