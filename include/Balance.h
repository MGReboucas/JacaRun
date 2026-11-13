#ifndef BALANCE_H
#define BALANCE_H

// Parametros experimentais do prototipo, sujeitos ao balanceamento do GDD.
namespace Balance {
inline constexpr double InitialSpeed = 10.0;
inline constexpr double ActionSpeed = 24.0; // Keeps jump/slide distance unchanged.
inline constexpr double MaximumSpeed = 60.0;
inline double DistancePressure(double meters) {
    if (meters <= 4000) return 0;
    if (meters < 10000) return (meters - 4000) / 9000.0;
    if (meters < 20000) return 2.0 / 3.0 + (meters - 10000) / 30000.0;
    return 1;
}
inline double RunSpeed(double distance) {
    const double base = InitialSpeed + distance * .012;
    return (base < ActionSpeed ? base : ActionSpeed) + 36.0 * DistancePressure(distance);
}
inline constexpr double SpeedPerMeter = 0.012;
inline constexpr double Gravity = -22.0;
inline constexpr double JumpForce = 9.5;
inline constexpr double DiveSpeed = 24.0;
inline constexpr double SlideSeconds = 0.9;
inline constexpr double GroundClearance = 0.7;
inline constexpr double PickupReach = 0.85;
inline constexpr double FishBiteReach = 0.85;
inline constexpr double FishBiteDistance = 4.0;
inline constexpr double ComboSeconds = 8.0;
inline constexpr int FoodsPerMultiplier = 3;
inline constexpr int MaximumMultiplier = 5;
inline constexpr int CrabCoins = 1;
inline constexpr int FishCoins = 2;
inline constexpr int RareFishCoins = 5;
inline constexpr double ShieldSeconds = 12.0;
inline constexpr double MagnetSeconds = 10.0;
inline constexpr double LightningRewardMeters = 5000.0;
inline constexpr double SpeedSeconds = 6.0;
inline constexpr double SpeedMultiplier = 2.0;
inline constexpr double LookAhead = 180.0;
inline constexpr double PhysicsStep = 1.0 / 120.0;
inline constexpr double TurnSeconds = 0.2;
inline constexpr int ExperiencePerLevel = 200;
}

#endif
