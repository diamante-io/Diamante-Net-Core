// Copyright 2014 Diamnet Development Foundation and contributors. Licensed
// under the Apache License, Version 2.0. See the COPYING file at the root
// of this distribution or at http://www.apache.org/licenses/LICENSE-2.0

#include "main/CommandLine.h"
#include "util/Logging.h"

#include "crypto/ShortHash.h"
#include <cstdlib>
#include <sodium/core.h>
#include <xdrpp/marshal.h>

INITIALIZE_EASYLOGGINGPP

namespace diamnet
{
static void
outOfMemory()
{
    std::fprintf(stderr, "Unable to allocate memory\n");
    std::fflush(stderr);
    std::abort();
}
}

int
main(int argc, char* const* argv)
{
    using namespace diamnet;

    // Abort when out of memory
    std::set_new_handler(outOfMemory);

    Logging::init();
    if (sodium_init() != 0)
    {
        LOG(FATAL) << "Could not initialize crypto";
        return 1;
    }
    shortHash::initialize();

    xdr::marshaling_stack_limit = 1000;

    return handleCommandLine(argc, argv);
}
