#pragma once

// Copyright 2017 Diamnet Development Foundation and contributors. Licensed
// under the Apache License, Version 2.0. See the COPYING file at the root
// of this distribution or at http://www.apache.org/licenses/LICENSE-2.0

#include <stdexcept>

namespace diamnet
{

class InvariantDoesNotHold : public std::runtime_error
{
  public:
    explicit InvariantDoesNotHold(std::string const& msg)
        : std::runtime_error{msg}
    {
    }
    virtual ~InvariantDoesNotHold() = default;
};
}
