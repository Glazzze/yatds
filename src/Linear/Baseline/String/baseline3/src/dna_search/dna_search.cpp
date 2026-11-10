#include "dna_search.hpp"

#include <algorithm>
#include <cctype>
#include <unordered_map>

/* START: dna_search implementation */

namespace {

const std::unordered_map<std::string, char> &codonTable()
{
    /* 标准遗传密码表（DNA 写法，T 代替 RNA 的 U） */
    static const std::unordered_map<std::string, char> table = {
        {"TTT", 'F'}, {"TTC", 'F'}, {"TTA", 'L'}, {"TTG", 'L'},
        {"CTT", 'L'}, {"CTC", 'L'}, {"CTA", 'L'}, {"CTG", 'L'},
        {"ATT", 'I'}, {"ATC", 'I'}, {"ATA", 'I'}, {"ATG", 'M'},
        {"GTT", 'V'}, {"GTC", 'V'}, {"GTA", 'V'}, {"GTG", 'V'},
        {"TCT", 'S'}, {"TCC", 'S'}, {"TCA", 'S'}, {"TCG", 'S'},
        {"CCT", 'P'}, {"CCC", 'P'}, {"CCA", 'P'}, {"CCG", 'P'},
        {"ACT", 'T'}, {"ACC", 'T'}, {"ACA", 'T'}, {"ACG", 'T'},
        {"GCT", 'A'}, {"GCC", 'A'}, {"GCA", 'A'}, {"GCG", 'A'},
        {"TAT", 'Y'}, {"TAC", 'Y'}, {"TAA", '*'}, {"TAG", '*'},
        {"CAT", 'H'}, {"CAC", 'H'}, {"CAA", 'Q'}, {"CAG", 'Q'},
        {"AAT", 'N'}, {"AAC", 'N'}, {"AAA", 'K'}, {"AAG", 'K'},
        {"GAT", 'D'}, {"GAC", 'D'}, {"GAA", 'E'}, {"GAG", 'E'},
        {"TGT", 'C'}, {"TGC", 'C'}, {"TGA", '*'}, {"TGG", 'W'},
        {"CGT", 'R'}, {"CGC", 'R'}, {"CGA", 'R'}, {"CGG", 'R'},
        {"AGT", 'S'}, {"AGC", 'S'}, {"AGA", 'R'}, {"AGG", 'R'},
        {"GGT", 'G'}, {"GGC", 'G'}, {"GGA", 'G'}, {"GGG", 'G'},
    };
    return table;
}

char complementBase(char base)
{
    switch (base) {
    case 'A':
        return 'T';
    case 'T':
        return 'A';
    case 'C':
        return 'G';
    case 'G':
        return 'C';
    default:
        return 'N';
    }
}

} // namespace

std::vector<int> dsaac::prefixFunction(const std::string &pattern)
{
    const int m = static_cast<int>(pattern.size());
    std::vector<int> pi(m, 0);
    int matched = 0;
    for (int i = 1; i < m; ++i) {
        while (matched > 0 && pattern[i] != pattern[matched])
            matched = pi[matched - 1];
        if (pattern[i] == pattern[matched])
            ++matched;
        pi[i] = matched;
    }
    return pi;
}

std::vector<std::size_t> dsaac::kmpFindAll(const std::string &text,
                                           const std::string &pattern,
                                           const std::vector<int> &pi)
{
    std::vector<std::size_t> positions;
    const std::size_t m = pattern.size();
    if (m == 0 || text.size() < m)
        return positions;

    std::size_t matched = 0;
    for (std::size_t i = 0; i < text.size(); ++i) {
        while (matched > 0 && text[i] != pattern[matched])
            matched = static_cast<std::size_t>(pi[matched - 1]);
        if (text[i] == pattern[matched])
            ++matched;
        if (matched == m) {
            positions.push_back(i - m + 1);
            /* 允许重叠：匹配成功后按前缀函数继续，而不是直接清零 */
            matched = static_cast<std::size_t>(pi[matched - 1]);
        }
    }
    return positions;
}

std::string dsaac::reverseComplement(const std::string &dna)
{
    std::string result;
    result.reserve(dna.size());
    for (auto it = dna.rbegin(); it != dna.rend(); ++it) {
        const char base = static_cast<char>(std::toupper(static_cast<unsigned char>(*it)));
        result.push_back(complementBase(base));
    }
    return result;
}

char dsaac::translateCodon(const std::string &codon)
{
    if (codon.size() != 3)
        return 'X';
    const auto it = codonTable().find(codon);
    if (it == codonTable().end())
        return 'X';
    return it->second;
}

std::string dsaac::translateFrame(const std::string &dna, int frame)
{
    std::string protein;
    if (frame < 0 || frame > 2)
        return protein;
    if (dna.size() < 3)
        return protein;

    protein.reserve((dna.size() - static_cast<std::size_t>(frame)) / 3);
    for (std::size_t i = static_cast<std::size_t>(frame); i + 2 < dna.size(); i += 3)
        protein.push_back(translateCodon(dna.substr(i, 3)));
    return protein;
}

std::vector<ProteinMatch> dsaac::findProteinMatches(const std::string &dna,
                                                    const std::string &protein)
{
    std::vector<ProteinMatch> matches;
    const int n = static_cast<int>(dna.size());
    const int m = static_cast<int>(protein.size());
    if (m == 0 || n < 3 * m)
        return matches;

    std::string upper(dna);
    std::transform(upper.begin(), upper.end(), upper.begin(),
                   [](unsigned char c) { return static_cast<char>(std::toupper(c)); });

    const std::vector<int> pi = prefixFunction(protein);
    const int blockLen = 3 * m;

    /* 正向链：frame ∈ {0,1,2}，匹配到的第 a 个氨基酸对应 [f+3a, f+3a+3m-1]（0-based） */
    for (int frame = 0; frame < 3; ++frame) {
        const std::string translated = translateFrame(upper, frame);
        for (std::size_t a : kmpFindAll(translated, protein, pi)) {
            const int left0 = frame + static_cast<int>(a) * 3;
            matches.push_back({left0 + 1, left0 + blockLen, '+', frame + 1});
        }
    }

    /* 反向互补链：rc 下标 k 对应原序列下标 n-1-k，区间的两个端点都要翻转 */
    const std::string rc = reverseComplement(upper);
    for (int frame = 0; frame < 3; ++frame) {
        const std::string translated = translateFrame(rc, frame);
        for (std::size_t a : kmpFindAll(translated, protein, pi)) {
            const int start = frame + static_cast<int>(a) * 3; // rc 上的起点（0-based）
            const int left0 = n - (start + blockLen);
            const int right0 = n - 1 - start;
            matches.push_back({left0 + 1, right0 + 1, '-', frame + 1});
        }
    }

    /* 按 L 升序；L 相同按 strand（'+' 在前）；再按 frame 升序 */
    std::sort(matches.begin(), matches.end(), [](const ProteinMatch &x,
                                                 const ProteinMatch &y) {
        if (x.l != y.l)
            return x.l < y.l;
        if (x.strand != y.strand)
            return x.strand < y.strand;
        return x.frame < y.frame;
    });
    return matches;
}

std::vector<long long> dsaac::answerQueries(
    const std::vector<ProteinMatch> &matches, int dnaLength, int proteinLength,
    const std::vector<std::pair<int, int>> &queries)
{
    std::vector<long long> answers;
    answers.reserve(queries.size());
    if (proteinLength <= 0 || dnaLength <= 0) {
        answers.assign(queries.size(), 0);
        return answers;
    }

    /*
     * 所有匹配片段长度都是 3m，因此“右端点 <= r”等价于“左端点 <= r-3m+1”。
     * prefix[x] 统计左端点 <= x 的匹配数，查询答案 = prefix[r-3m+1] - prefix[l-1]。
     */
    std::vector<long long> prefix(static_cast<std::size_t>(dnaLength) + 1, 0);
    for (const ProteinMatch &match : matches) {
        if (match.l >= 1 && match.l <= dnaLength)
            ++prefix[static_cast<std::size_t>(match.l)];
    }
    for (int i = 1; i <= dnaLength; ++i)
        prefix[static_cast<std::size_t>(i)] += prefix[static_cast<std::size_t>(i - 1)];

    const long long blockLen = 3LL * proteinLength;
    for (const auto &query : queries) {
        long long l = query.first;
        long long r = query.second;
        if (l < 1)
            l = 1;
        if (r > dnaLength)
            r = dnaLength;
        if (l > r) {
            answers.push_back(0);
            continue;
        }
        long long hi = r - blockLen + 1; // 允许的最大左端点
        if (hi > dnaLength)
            hi = dnaLength;
        if (hi < l) {
            answers.push_back(0);
            continue;
        }
        answers.push_back(prefix[static_cast<std::size_t>(hi)] -
                          prefix[static_cast<std::size_t>(l - 1)]);
    }
    return answers;
}

/* END */