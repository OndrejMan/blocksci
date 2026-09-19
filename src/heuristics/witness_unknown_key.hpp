#ifndef BLOCKSCI_HEURISTICS_WITNESS_UNKNOWN_KEY_HPP
#define BLOCKSCI_HEURISTICS_WITNESS_UNKNOWN_KEY_HPP

#include <cstdint>
#include <string>

namespace blocksci {
    namespace heuristics {
        namespace detail {
            template <typename Iterator>
            std::string witnessUnknownAddressKey(uint8_t witnessVersion, Iterator begin, Iterator end) {
                // Preserve both the witness version and every byte of the
                // 2-40 byte witness program, including embedded zero bytes.
                std::string key(1, static_cast<char>(witnessVersion));
                key.append(begin, end);
                return key;
            }
        }
    }
}

#endif
