// perlin.cpp — Bruit de Perlin 2D en C++17, sans dépendance externe.
//
// Basé sur l'algorithme "Improved Perlin Noise" de Ken Perlin (2002),
// adapté en deux dimensions : table de permutation, fonction de lissage
// (fade), interpolation bilinéaire lissée, et gradients pseudo-aléatoires.
//
// Compilation :
//   g++ -std=c++17 -O2 -Wall -Wextra perlin.cpp -o perlin
// Exécution :
//   ./perlin          -> génère perlin.pgm + aperçu ASCII
//   ./perlin --test   -> exécute uniquement les tests unitaires

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <random>
#include <string>

/// Générateur de bruit de Perlin 2D.
///
/// Construire une instance avec un seed : le même seed reproduira
/// toujours exactement le même bruit (std::mt19937 est entièrement
/// spécifié par le standard C++, donc déterministe et portable).
class PerlinNoise {
public:
    explicit PerlinNoise(uint32_t seed = 0) {
        std::array<uint8_t, 256> p{};
        for (int i = 0; i < 256; ++i) {
            p[static_cast<size_t>(i)] = static_cast<uint8_t>(i);
        }

        std::mt19937 rng(seed);
        std::shuffle(p.begin(), p.end(), rng);

        // Table dupliquée (512 = 2 x 256) pour éviter un modulo lors
        // des accès aux coins de la grille.
        for (int i = 0; i < 512; ++i) {
            perm_[static_cast<size_t>(i)] = p[static_cast<size_t>(i % 256)];
        }
    }

    /// Valeur de bruit de Perlin 2D au point (x, y).
    /// Résultat compris (en pratique) dans l'intervalle [-1.0, 1.0].
    double noise2d(double x, double y) const {
        int xi = wrap(static_cast<int>(std::floor(x)));
        int yi = wrap(static_cast<int>(std::floor(y)));

        double xf = x - std::floor(x);
        double yf = y - std::floor(y);

        double u = fade(xf);
        double v = fade(yf);

        // Hash des 4 coins de la cellule à partir de la table de permutation.
        uint8_t aa = at(xi, yi);
        uint8_t ab = at(xi, yi + 1);
        uint8_t ba = at(xi + 1, yi);
        uint8_t bb = at(xi + 1, yi + 1);

        // Bruit aux 4 coins, puis double interpolation (bilinéaire lissée).
        double x1 = lerp(grad(aa, xf, yf), grad(ba, xf - 1.0, yf), u);
        double x2 = lerp(grad(ab, xf, yf - 1.0), grad(bb, xf - 1.0, yf - 1.0), u);

        return lerp(x1, x2, v);
    }

    /// Bruit fractal (fBm, "fractional Brownian motion") : somme de
    /// plusieurs octaves à fréquences croissantes et amplitudes
    /// décroissantes. Résultat normalisé pour rester dans [-1.0, 1.0].
    double fbm(double x, double y, int octaves, double persistence) const {
        double total = 0.0;
        double frequency = 1.0;
        double amplitude = 1.0;
        double max_value = 0.0;

        for (int i = 0; i < octaves; ++i) {
            total += noise2d(x * frequency, y * frequency) * amplitude;
            max_value += amplitude;
            amplitude *= persistence;
            frequency *= 2.0;
        }

        return total / max_value;
    }

private:
    std::array<uint8_t, 512> perm_{};

    // Accès à la table doublement indexée perm[perm[xi] + yi],
    // avec xi et yi repliés sur 0..255 au préalable.
    uint8_t at(int xi_raw, int yi_raw) const {
        int xi = wrap(xi_raw);
        int yi = wrap(yi_raw);
        return perm_[static_cast<size_t>(perm_[static_cast<size_t>(xi)]) + static_cast<size_t>(yi)];
    }

    static int wrap(int v) {
        int m = v % 256;
        return m < 0 ? m + 256 : m;
    }

    // Fonction de lissage de Perlin : 6t^5 - 15t^4 + 10t^3.
    // Dérivées première et seconde nulles en 0 et 1 : pas de
    // discontinuité visible aux bords des cellules.
    static double fade(double t) {
        return t * t * t * (t * (t * 6.0 - 15.0) + 10.0);
    }

    static double lerp(double a, double b, double t) {
        return a + t * (b - a);
    }

    // Produit scalaire avec l'un des 8 vecteurs gradients possibles.
    static double grad(uint8_t hash, double x, double y) {
        switch (hash & 7) {
        case 0: return  x + y;
        case 1: return -x + y;
        case 2: return  x - y;
        case 3: return -x - y;
        case 4: return  x;
        case 5: return -x;
        case 6: return  y;
        default: return -y;
        }
    }
};