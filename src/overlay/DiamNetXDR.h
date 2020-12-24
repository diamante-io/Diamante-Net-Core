#pragma once
#include "xdr/DiamNet-ledger-entries.h"
#include "xdr/DiamNet-ledger.h"
#include "xdr/DiamNet-overlay.h"
#include "xdr/DiamNet-transaction.h"
#include "xdr/DiamNet-types.h"

namespace DiamNet
{

std::string xdr_printer(const PublicKey& pk);
}
