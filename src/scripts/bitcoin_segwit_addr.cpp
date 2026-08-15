/* Copyright (c) 2017 Pieter Wuille
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "bitcoin_segwit_addr.hpp"

#include "bitcoin_bech32.hpp"

#include <internal/data_configuration.hpp>

namespace {

typedef std::vector<uint8_t> segwit_data;

/** Convert from one power-of-2 number base to another. */
template<int frombits, int tobits, bool pad>
bool convertbits(segwit_data& out, const segwit_data& in) {
    int acc = 0;
    int bits = 0;
    const int maxv = (1 << tobits) - 1;
    const int max_acc = (1 << (frombits + tobits - 1)) - 1;
    for (size_t i = 0; i < in.size(); ++i) {
        int value = in[i];
        acc = ((acc << frombits) | value) & max_acc;
        bits += frombits;
        while (bits >= tobits) {
            bits -= tobits;
            out.push_back((acc >> bits) & maxv);
        }
    }
    if (pad) {
        if (bits) out.push_back((acc << (tobits - bits)) & maxv);
    } else if (bits >= frombits || ((acc << (tobits - bits)) & maxv)) {
        return false;
    }
    return true;
}

/** Pick the bech32 variant a witness version has to be encoded with. */
bech32::Encoding encodingForWitnessVersion(int witver) {
    return witver == 0 ? bech32::Encoding::BECH32 : bech32::Encoding::BECH32M;
}

/** Witness versions are pushed as OP_0 through OP_16, so nothing above 16 exists. */
bool isKnownWitnessVersion(int witver) {
    return witver >= 0 && witver <= 16;
}

/** Witness programs are 2 to 40 bytes long. */
bool hasValidProgramLength(const segwit_data& program) {
    return program.size() >= 2 && program.size() <= 40;
}

/** Version 0 defines exactly two programs: 20 bytes for P2WPKH and 32 bytes for P2WSH. */
bool hasValidVersionZeroLength(int witver, const segwit_data& program) {
    if (witver != 0) return true;
    return program.size() == 20 || program.size() == 32;
}

/** Version 0 is checksummed with Bech32, every later version with Bech32m. */
bool usesRequiredEncoding(int witver, bech32::Encoding encoding) {
    return encoding == encodingForWitnessVersion(witver);
}

} // namespace

namespace segwit_addr {

/** Decode a SegWit address. */
std::pair<int, segwit_data> decode(const std::string& hrp, const std::string& addr) {
    auto dec = bech32::decodeWithEncoding(addr);
    const auto &decoded_hrp = std::get<0>(dec);
    const auto &decoded_data = std::get<1>(dec);
    const auto encoding = std::get<2>(dec);
    if (decoded_hrp != hrp || decoded_data.size() < 1) return std::make_pair(-1, segwit_data());
    segwit_data program;
    const int witness_version = decoded_data[0];
    if (!convertbits<5, 8, false>(program, segwit_data(decoded_data.begin() + 1, decoded_data.end())) ||
        !isKnownWitnessVersion(witness_version) ||
        !hasValidProgramLength(program) ||
        !hasValidVersionZeroLength(witness_version, program) ||
        !usesRequiredEncoding(witness_version, encoding)) {
        return std::make_pair(-1, segwit_data());
    }
    return std::make_pair(witness_version, program);
}

/** Encode a SegWit address. */
std::string encode(const std::string& hrp, int witver, const segwit_data& witprog) {
    segwit_data enc;
    enc.push_back(static_cast<unsigned char>(witver));
    convertbits<8, 5, true>(enc, witprog);
    std::string ret = bech32::encode(hrp, enc, encodingForWitnessVersion(witver));
    if (decode(hrp, ret).first == -1) return "";
    return ret;
}

std::string encode(const blocksci::ChainConfiguration &config, int witver, const segwit_data& witprog) {
    return encode(config.segwitPrefix, witver, witprog);
}

} // namespace segwit_addr
