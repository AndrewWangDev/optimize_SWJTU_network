#include <iostream>
#include <vector>
#include <string>
#include <sstream>
#include <cstdint>
#include <algorithm>
#include <cmath>
#include <iomanip>

struct IPNetwork {
    uint32_t network;
    int prefix;
    
    IPNetwork(uint32_t net, int pre) : network(net), prefix(pre) {}
    
    // Convert IP string to uint32
    static uint32_t parseIP(const std::string& ip) {
        uint32_t result = 0;
        int parts[4] = {0};
        sscanf(ip.c_str(), "%d.%d.%d.%d", &parts[0], &parts[1], &parts[2], &parts[3]);
        result = (parts[0] << 24) | (parts[1] << 16) | (parts[2] << 8) | parts[3];
        return result;
    }
    
    // Convert uint32 to IP string
    static std::string ipToString(uint32_t ip) {
        std::stringstream ss;
        ss << ((ip >> 24) & 0xFF) << "."
           << ((ip >> 16) & 0xFF) << "."
           << ((ip >> 8) & 0xFF) << "."
           << (ip & 0xFF);
        return ss.str();
    }
    
    // Parse CIDR notation
    static IPNetwork fromCIDR(const std::string& cidr) {
        size_t slashPos = cidr.find('/');
        if (slashPos != std::string::npos) {
            std::string ipPart = cidr.substr(0, slashPos);
            int prefixLen = std::stoi(cidr.substr(slashPos + 1));
            uint32_t ip = parseIP(ipPart);
            uint32_t mask = (prefixLen == 0) ? 0 : (0xFFFFFFFFU << (32 - prefixLen));
            uint32_t network = ip & mask;
            return IPNetwork(network, prefixLen);
        } else {
            // Single IP address
            uint32_t ip = parseIP(cidr);
            return IPNetwork(ip, 32);
        }
    }
    
    // Get network mask
    uint32_t getMask() const {
        if (prefix == 0) return 0;
        return (0xFFFFFFFFU << (32 - prefix));
    }
    
    // Get broadcast address
    uint32_t getBroadcast() const {
        uint32_t mask = getMask();
        return network | (~mask);
    }
    
    // Check if two networks overlap
    bool overlaps(const IPNetwork& other) const {
        uint32_t mask1 = getMask();
        uint32_t mask2 = other.getMask();
        return (network & mask1) == (other.network & mask2) ||
               (network & mask2) == (other.network & mask2) ||
               (other.network & mask1) == (network & mask1);
    }
    
    // Address exclusion - split network by excluding the given network
    std::vector<IPNetwork> addressExclude(const IPNetwork& exclude) const {
        std::vector<IPNetwork> result;
        
        uint32_t mask = getMask();
        uint32_t excMask = exclude.getMask();
        
        // If exclude network is not actually within this network, return this network
        if ((network & mask) != (exclude.network & mask)) {
            // Check if exclude is within this network
            if ((exclude.network & mask) != (network & mask)) {
                result.push_back(*this);
                return result;
            }
        }
        
        // Generate subnets by increasing prefix length
        for (int p = prefix + 1; p <= 32; ++p) {
            uint32_t subMask = (p == 0) ? 0 : (0xFFFFFFFFU << (32 - p));
            uint32_t subnet1 = network & subMask;
            uint32_t subnet2 = subnet1 | ~subMask;
            
            IPNetwork net1(subnet1, p);
            IPNetwork net2(subnet2, p);
            
            if (!net1.overlaps(exclude)) {
                result.push_back(net1);
            }
            if (!net2.overlaps(exclude)) {
                result.push_back(net2);
            }
            
            if (result.size() >= 2) break;
        }
        
        if (result.empty()) {
            result.push_back(*this);
        }
        
        return result;
    }
    
    // Convert to string
    std::string toString() const {
        return ipToString(network) + "/" + std::to_string(prefix);
    }
    
    // Compare for sorting
    bool operator<(const IPNetwork& other) const {
        if (network != other.network) return network < other.network;
        return prefix < other.prefix;
    }
    
    bool operator==(const IPNetwork& other) const {
        return network == other.network && prefix == other.prefix;
    }
};

int main() {
    std::string start = "0.0.0.0/0";
    std::vector<std::string> exclude = {"10.0.0.0/8", "172.16.0.0/12", "192.168.0.0/16", "6.6.6.6", "8.8.8.8"};
    
    std::vector<IPNetwork> result;
    result.push_back(IPNetwork::fromCIDR(start));
    
    for (const auto& x : exclude) {
        IPNetwork n = IPNetwork::fromCIDR(x);
        
        std::vector<IPNetwork> newResult;
        for (const auto& y : result) {
            if (y.overlaps(n)) {
                auto subnets = y.addressExclude(n);
                newResult.insert(newResult.end(), subnets.begin(), subnets.end());
            } else {
                newResult.push_back(y);
            }
        }
        result = newResult;
    }
    
    // Sort results
    std::sort(result.begin(), result.end());
    
    // Remove duplicates
    result.erase(std::unique(result.begin(), result.end()), result.end());
    
    // Print results
    for (size_t i = 0; i < result.size(); ++i) {
        if (i > 0) std::cout << ",";
        std::cout << result[i].toString();
    }
    std::cout << std::endl;
    
    return 0;
}
