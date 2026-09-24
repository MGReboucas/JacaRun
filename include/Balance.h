#ifndef BALANCE_H
#define BALANCE_H

// Parametros experimentais do prototipo, sujeitos ao balanceamento do GDD.
namespace Balance {
inline constexpr double InitialSpeed = 10.0;
inline constexpr double ActionSpeed = 24.0; // Keeps jump/slide distance unchanged.
inline constexpr double MaximumSpeed = 60.0;
inline double ScorePressure(double score) { return score <= 0 ? 0 : score >= 100000 ? 1 : score / 100000.0; }
inline double RunSpeed(double distance, double score) {
    const double base = InitialSpeed + distance * .012;
    return (base < ActionSpeed ? base : ActionSpeed) + 36.0 * ScorePressure(score);
}
inline constexpr double SpeedPerMeter = 0.012;
inline constexpr double Gravity = -22.0;
inline constexpr double JumpForce = 9.5;
inline constexpr double SlideSeconds = 0.9;
inline constexpr double GroundClearance = 0.7;
inline constexpr double PickupReach = 0.85;
inline constexpr double ComboSeconds = 8.0;
inline constexpr int FoodsPerMultiplier = 3;
inline constexpr int MaximumMultiplier = 5;
inline constexpr int CrabPoints = 20;
inline constexpr int FishPoints = 30;
inline constexpr int RareFishPoints = 100;
inline constexpr double ShieldSeconds = 12.0;
inline constexpr double MagnetSeconds = 10.0;
inline constexpr double LookAhead = 180.0;
inline constexpr double PhysicsStep = 1.0 / 120.0;
inline constexpr double TurnSeconds = 0.2;
inline constexpr int ExperiencePerLevel = 200;
}

#endif
