#pragma once
#include "xdr/HcNet-ledger-entries.h"
#include "xdr/HcNet-ledger.h"
#include "xdr/HcNet-overlay.h"
#include "xdr/HcNet-transaction.h"
#include "xdr/HcNet-types.h"

namespace HcNet
{

std::string xdr_printer(const PublicKey& pk);
}
