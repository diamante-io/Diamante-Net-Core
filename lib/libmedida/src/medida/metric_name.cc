//
// Copyright (c) 2012 Daniel Lundin
//

#include "medida/metric_name.h"

#include <stdexcept>

namespace medida {

class MetricName::Impl {
 public:
  Impl(const std::string &domain, const std::string &type, const std::string &name, const std::string &scope = "");
  ~Impl();
  std::string domain() const;
  std::string type() const;
  std::string name() const;
  std::string scope() const;
  std::string ToString() const;
  bool has_scope() const;
  bool operator==(const Impl& other) const;
  bool operator!=(const Impl& other) const;
  bool operator<(const Impl& other) const;
  bool operator>(const Impl& other) const;
 private:
  const std::string domain_;
  const std::string type_;
  const std::string name_;
  const std::string scope_;
  const std::string repr_;
};


MetricName::MetricName(const std::string &domain, const std::string &type,
    const std::string &name, const std::string &scope)
    : impl_ {new MetricName::Impl {domain, type, name, scope}} {
}


MetricName::MetricName(const MetricName& other)
    : impl_ {new MetricName::Impl(*other.impl_)} {
}


MetricName::~MetricName() {
}


std::string MetricName::domain() const {
  return impl_->domain();
}


std::string MetricName::type() const {
  return impl_->type();
}


std::string MetricName::name() const {
  return impl_->name();
}


std::string MetricName::scope() const {
  return impl_->scope();
}


std::string MetricName::ToString() const {
  return impl_->ToString();
}


bool MetricName::has_scope() const {
  return impl_->has_scope();
}


bool MetricName::operator==(const MetricName &other) const {
  return *impl_ == *other.impl_;
}


bool MetricName::operator!=(const MetricName &other) const {
  return *impl_ != *other.impl_;
}


bool MetricName::operator<(const MetricName& other) const {
  return *impl_ < *other.impl_;
}


bool MetricName::operator>(const MetricName& other) const {
  return *impl_ > *other.impl_;
}


// === Implementation ===


MetricName::Impl::Impl(const std::string &domain, const std::string &type,
    const std::string &name, const std::string &scope)
    : domain_ (domain),
      type_   (type),
      name_   (name),
      scope_  (scope),
      repr_   (domain + "." + type + "." + name  + (scope.empty() ? "" : "." + scope)) {
  if (domain.empty()) {
    throw std::invalid_argument("domain must be non-empty");
  }
  if (type.empty()) {
    throw std::invalid_argument("type must be non-empty");
  }
  if (name.empty()) {
    throw std::invalid_argument("name must be non-empty");
  }
}


MetricName::Impl::~Impl() {
}


std::string MetricName::Impl::domain() const {
  return domain_;
}


std::string MetricName::Impl::type() const {
  return type_;
}


std::string MetricName::Impl::name() const {
  return name_;
}


std::string MetricName::Impl::scope() const {
  return scope_;
}


std::string MetricName::Impl::ToString() const {
  return repr_;
}


bool MetricName::Impl::has_scope() const {
  return !scope_.empty();
}


bool MetricName::Impl::operator==(const Impl &other) const {
  return repr_ == other.repr_;
}


bool MetricName::Impl::operator!=(const Impl &other) const {
  return repr_ != other.repr_;
}


bool MetricName::Impl::operator<(const Impl& other) const {
  return repr_ < other.repr_;
}


bool MetricName::Impl::operator>(const Impl& other) const {
  return repr_ > other.repr_;
}


} // name space medida
