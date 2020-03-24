//
// Copyright (c) 2012 Daniel Lundin
//

#ifndef MEDIDA_SUMMARIZABLE_INTERFACE_H_
#define MEDIDA_SUMMARIZABLE_INTERFACE_H_

namespace medida {

class SummarizableInterface {
public:
  virtual ~SummarizableInterface() {};
  virtual double max() const = 0;
  virtual double min() const = 0;
  virtual double mean() const = 0;
  virtual double std_dev() const = 0;
  virtual double sum() const = 0;
};

} // namespace medida

#endif // MEDIDA_SUMMARIZABLE_INTERFACE_H_
