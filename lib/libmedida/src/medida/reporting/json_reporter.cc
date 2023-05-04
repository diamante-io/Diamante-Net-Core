//
// Copyright (c) 2012 Daniel Lundin
//

#include "medida/reporting/json_reporter.h"

#include <chrono>
#include <ctime>
#include <mutex>
#include <sstream>
#ifdef _WIN32
#include <Winsock2.h>
#else
#include <sys/utsname.h>
#endif

#include "medida/reporting/util.h"

namespace medida {
namespace reporting {

class JsonReporter::Impl {
 public:
  Impl(JsonReporter& self, MetricsRegistry &registry);
  ~Impl();
  void Process(Counter& counter);
  void Process(Meter& meter);
  void Process(Histogram& histogram);
  void Process(Timer& timer);
  void Process(Buckets &buckets);
  std::string Report();
 private:
  JsonReporter& self_;
  medida::MetricsRegistry& registry_;
  mutable std::mutex mutex_;
  std::stringstream out_;
  std::string uname_;
};


JsonReporter::JsonReporter(MetricsRegistry &registry)
    : impl_ {new JsonReporter::Impl {*this, registry}} {
}


JsonReporter::~JsonReporter() {
}


std::string JsonReporter::Report() {
  return impl_->Report();
}


void JsonReporter::Process(Counter& counter) {
  impl_->Process(counter);
}


void JsonReporter::Process(Meter& meter) {
  impl_->Process(meter);
}


void JsonReporter::Process(Histogram& histogram) {
  impl_->Process(histogram);
}


void JsonReporter::Process(Timer& timer) {
  impl_->Process(timer);
}

void
JsonReporter::Process(Buckets& buckets)
{
    impl_->Process(buckets);
}


// === Implementation ===


JsonReporter::Impl::Impl(JsonReporter& self, MetricsRegistry &registry)
    : self_     (self),
      registry_ (registry) {
#ifdef _WIN32
	char nameBuf[128];
	if (gethostname(nameBuf, sizeof(nameBuf)) == 0)
	{
		uname_ = std::string(nameBuf);
	}
	else
	{
		uname_ = std::string("localhost");
	}
#else
  utsname name;
  uname_ = {uname(&name) ? "localhost" : name.nodename};
#endif
}


JsonReporter::Impl::~Impl() {
}


std::string JsonReporter::Impl::Report() {
  auto t = std::time(NULL);
  char mbstr[32] = "";

  std::tm tm;
#ifdef _WIN32
    // On Win32 this is returns a thread-local and there's no _r variant.
    std::tm *tmPtr = gmtime(&t);
    tm = *tmPtr;
#else
    // On unix the _r variant uses a local output, so is threadsafe.
    gmtime_r(&t, &tm);
#endif

  std::strftime(mbstr, 32, "%FT%TZ", &tm);
  std::lock_guard<std::mutex> lock {mutex_};
  out_.str("");
  out_.clear();
  out_ << "{" << std::endl
       << "\"ts\":\"" << mbstr << "\"," << std::endl
       << "\"uname\":\"" << uname_ << "\"," << std::endl
       << "\"metrics\":{" << std::endl;
  auto first = true;
  for (auto& kv : registry_.GetAllMetrics()) {
    auto name = kv.first;
    auto metric = kv.second;
    if (first) {
      first = false;
    } else {
      out_ << ",";
    }
    out_ << "\"" << name.ToString() << "\":{" << std::endl;
    metric->Process(self_);
    out_ << "}" << std::endl;
  }
  out_ << "}"    // metrics
       << "}";  // top
  return out_.str();
}


void JsonReporter::Impl::Process(Counter& counter) {
  out_ << "\"type\":\"counter\"," << std::endl;
  out_ << "\"count\":" << counter.count() << std::endl;
}


void JsonReporter::Impl::Process(Meter& meter) {
  auto event_type = meter.event_type();
  auto unit = FormatRateUnit(meter.rate_unit());
  out_ << "\"type\":\"meter\"," << std::endl
       << "\"count\":" << meter.count() << "," << std::endl
       << "\"event_type\":\"" << event_type << "\"," << std::endl
       << "\"rate_unit\":\"" << unit << "\"," << std::endl
       << "\"mean_rate\":" << meter.mean_rate() << "," << std::endl
       << "\"1_min_rate\":" << meter.one_minute_rate() << "," << std::endl
       << "\"5_min_rate\":" << meter.five_minute_rate() << "," << std::endl
       << "\"15_min_rate\":" << meter.fifteen_minute_rate() << std::endl;
}


void JsonReporter::Impl::Process(Histogram& histogram) {
  auto snapshot = histogram.GetSnapshot();
#ifdef _WIN32
#undef min
#undef max
#endif
  out_ << "\"type\":\"histogram\"," << std::endl
       << "\"count\":" << histogram.count() << "," << std::endl
       << "\"min\":" << histogram.min() << "," << std::endl
       << "\"max\":" << histogram.max() << "," << std::endl
       << "\"mean\":" << histogram.mean() << "," << std::endl
       << "\"stddev\":" << histogram.std_dev() << "," << std::endl
       << "\"sum\":" << histogram.sum() << "," << std::endl
       << "\"median\":" << snapshot.getMedian() << "," << std::endl
       << "\"75%\":" << snapshot.get75thPercentile() << "," << std::endl
       << "\"95%\":" << snapshot.get95thPercentile() << "," << std::endl
       << "\"98%\":" << snapshot.get98thPercentile() << "," << std::endl
       << "\"99%\":" << snapshot.get99thPercentile() << "," << std::endl
       << "\"99.9%\":" << snapshot.get999thPercentile() << std::endl;
}


void JsonReporter::Impl::Process(Timer& timer) {
  auto snapshot = timer.GetSnapshot();
  auto rate_unit = FormatRateUnit(timer.rate_unit());
  auto duration_unit = FormatRateUnit(timer.duration_unit());
  out_ << "\"type\":\"timer\"," << std::endl
       << "\"count\":" << timer.count() << "," << std::endl
       << "\"event_type\":\"" << timer.event_type() << "\"," << std::endl
       << "\"rate_unit\":\"" << rate_unit << "\"," << std::endl
       << "\"mean_rate\":" << timer.mean_rate() << "," << std::endl
       << "\"1_min_rate\":" << timer.one_minute_rate() << "," << std::endl
       << "\"5_min_rate\":" << timer.five_minute_rate() << "," << std::endl
       << "\"15_min_rate\":" << timer.fifteen_minute_rate() << "," << std::endl
       << "\"duration_unit\":\"" << duration_unit << "\"," << std::endl
       << "\"min\":" << timer.min() << "," << std::endl
       << "\"max\":" << timer.max() << "," << std::endl
       << "\"mean\":" << timer.mean() << "," << std::endl
       << "\"stddev\":" << timer.std_dev() << "," << std::endl
       << "\"sum\":" << timer.sum() << "," << std::endl
       << "\"median\":" << snapshot.getMedian() << "," << std::endl
       << "\"75%\":" << snapshot.get75thPercentile() << "," << std::endl
       << "\"95%\":" << snapshot.get95thPercentile() << "," << std::endl
       << "\"98%\":" << snapshot.get98thPercentile() << "," << std::endl
       << "\"99%\":" << snapshot.get99thPercentile() << "," << std::endl
       << "\"99.9%\":" << snapshot.get999thPercentile() << std::endl;
}

void
JsonReporter::Impl::Process(Buckets& buckets)
{
    auto& bucketData = buckets.getBuckets();
    auto boundary_unit = FormatRateUnit(buckets.boundary_unit());

    out_ << "\"type\":\"buckets\"," << std::endl
         << "\"boundary_unit\":\"" << boundary_unit << "\"," << std::endl
         << "\"buckets\": [" << std::endl;
    for (auto it =bucketData.begin(); it != bucketData.end(); ++it)
    {
        auto&b = *it;
        if (it != bucketData.begin())
        {
            out_ << ",";
        }
        out_ << "{\n\"boundary\": " << b.first << "," << std::endl;
        b.second->Process(self_);
        out_ << "}" << std::endl;
    }
    out_ << "]" << std::endl;
}

} // namespace reporting
} // namespace medida
