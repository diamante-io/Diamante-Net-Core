// Copyright 2018 DiamNet Development Foundation and contributors. Licensed
// under the Apache License, Version 2.0. See the COPYING file at the root
// of this distribution or at http://www.apache.org/licenses/LICENSE-2.0

#include "MetricResetter.h"

namespace DiamNet
{

void
MetricResetter::Process(medida::Counter& counter)
{
    counter.clear();
}

void
MetricResetter::Process(medida::Meter& meter)
{
    meter.Clear();
}

void
MetricResetter::Process(medida::Histogram& histogram)
{
    histogram.Clear();
}

void
MetricResetter::Process(medida::Timer& timer)
{
    timer.Clear();
}
}
