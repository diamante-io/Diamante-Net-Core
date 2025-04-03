# DCP (Diamante Consensus Protocol)

The DCP subsystem is an abstract implementation of DCP, a protocol for federated
byzantine agreement, intended to drive a distributed system built around the
"replicated state machine" formalism. DCP is defined without reference to any
particular interpretation of the concepts of "slot" or "value", nor any
particular network communication system or replicated state machine.

This separation from the rest of the system is intended to make the
implementation of DCP easier to model, compare to the paper describing the
protocol, audit for correctness, and extract for reuse in different programs at
a later date.


