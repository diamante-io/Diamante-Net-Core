% HcNet-core(1)
% HcNet Development Foundation
%

# NAME

HcNet-core - Core daemon for HcNet payment network

# SYNOPSYS

HcNet-core [OPTIONS]

# DESCRIPTION

HcNet is a decentralized, federated peer-to-peer network that allows
people to send payments in any asset anywhere in the world
instantaneously, and with minimal fee. `HcNet-core` is the core
component of this network. `HcNet-core` is a C++ implementation of
the HcNet Consensus Protocol configured to construct a chain of
ledgers that are guaranteed to be in agreement across all the
participating nodes at all times.

## Configuration file

In most modes of operation, HcNet-core requires a configuration
file.  By default, it looks for a file called `HcNet-core.cfg` in
the current working directory, but this default can be changed by the
`--conf` command-line option.  The configuration file is in TOML
syntax.  The full set of supported directives can be found in
`%prefix%/share/doc/HcNet-core_example.cfg`.

%commands%

# EXAMPLES

See `%prefix%/share/doc/*.cfg` for some example HcNet-core
configuration files

# FILES

HcNet-core.cfg
:   Configuration file (in current working directory by default)

# SEE ALSO

<https://www.HcNet.org/developers/HcNet-core/software/admin.html>
:   HcNet-core administration guide

<https://www.HcNet.org>
:   Home page of HcNet development foundation

# BUGS

Please report bugs using the github issue tracker:\
<https://github.com/HcNet/HcNet-core/issues>
