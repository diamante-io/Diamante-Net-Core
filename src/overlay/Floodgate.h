#pragma once

// Copyright 2014 Diamnet Development Foundation and contributors. Licensed
// under the Apache License, Version 2.0. See the COPYING file at the root
// of this distribution or at http://www.apache.org/licenses/LICENSE-2.0

#include "overlay/Peer.h"
#include "overlay/DiamnetXDR.h"
#include <map>

/**
 * FloodGate keeps track of which peers have sent us which broadcast messages,
 * in order to ensure that for each broadcast message M and for each peer P, we
 * either send M to P once (and only once), or receive M _from_ P (thereby
 * inhibit sending M to P at all).
 *
 * The broadcast message types are TRANSACTION and SCP_MESSAGE.
 *
 * All messages are marked with the ledger sequence number to which they
 * relate, and all flood-management information for a given ledger number
 * is purged from the FloodGate when the ledger closes.
 */

namespace medida
{
class Counter;
}

namespace diamnet
{

class Floodgate
{
    class FloodRecord
    {
      public:
        typedef std::shared_ptr<FloodRecord> pointer;

        uint32_t mLedgerSeq;
        DiamnetMessage mMessage;
        std::set<std::string> mPeersTold;

        FloodRecord(DiamnetMessage const& msg, uint32_t ledger,
                    Peer::pointer peer);
    };

    std::map<Hash, FloodRecord::pointer> mFloodMap;
    Application& mApp;
    medida::Counter& mFloodMapSize;
    medida::Meter& mSendFromBroadcast;
    bool mShuttingDown;

  public:
    Floodgate(Application& app);
    // Floodgate will be cleared after every ledger close
    void clearBelow(uint32_t currentLedger);
    // returns true if this is a new record
    // fills msgID with msg's hash
    bool addRecord(DiamnetMessage const& msg, Peer::pointer fromPeer,
                   Hash& msgID);

    void broadcast(DiamnetMessage const& msg, bool force);

    // returns the list of peers that sent us the item with hash `msgID`
    // NB: `msgID` is the hash of a `DiamnetMessage`
    std::set<Peer::pointer> getPeersKnows(Hash const& msgID);

    // removes the record corresponding to `msgID`
    // `msgID` corresponds to a `DiamnetMessage`
    void forgetRecord(Hash const& msgID);

    void shutdown();

    void updateRecord(DiamnetMessage const& oldMsg,
                      DiamnetMessage const& newMsg);
};
}
