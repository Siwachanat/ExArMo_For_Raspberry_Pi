/*
  Elbow Exoskeleton — Effort-Adaptive Admittance Control   (FULL FIRMWARE v2.9t-DV1)
  ExArMo BLDC Driver V1 + SimpleFOC 2.4.0, rewound 5008 BLDC (40T-Y, 123 rpm/V), 8.68:1 capstan.

  [DV1, 6 Nov 2026] v2.9t PORTED TO THE ExArMo BLDC DRIVER V1 BOARD.
                  The control law (estimators, admittance, homing, passive, lab velocity,
                  thermal model, thermistor limits), the constants (N 8.68,
                  R 0.47, dz 0.15), the 13-field telemetry and every command / parameter
                  letter are v2.9t's, untouched: ExArMoGui_v2.9 (v2.9t), ExArMoLab_v2.9,
                  ExArMoRehab_v2.9 and exarmolab.py work as they are. 'L' still reports
                  fw=v2.9t (what the GUIs check) followed by hw=DV1 and the board fields.
                  Hardware layer (include/exarmo_driverv1.h, shared with LabTest-DV1):
                    - DRV8301 -> DRV8320S, configured over SPI3 at boot (3x PWM mode,
                      IDRIVE 120/240 mA, dead time 100 ns, VDS 0.13 V latched OCP).
                      ENABLE stays high (in sleep the DRV8320S turns DVDD off);
                      IDLE/FAULT = INLx low = all FETs off = arm free, as in v2.6.
                      nFAULT (PB12) checked every control tick -> FAULT + EV,DRVFAULT.
                      The settings are re-checked at 1 Hz (VM brown-out = coast + rewrite).
                    - Current sense: inline INA240A1 x20 on 3 mOhm, phases A/B/C on
                      PC0/PC1/PC2 (MKS: low-side x10 on 0.5 mOhm, B/C). Polarity/phase
                      order verified at boot (EV,CSALIGN). The sense is valid with the
                      bridge off, so the IDLE "garbage current" of the DRV8301 is gone.
                    - Encoder: AS5047P board on J11 / SPI1 (PB3/PB4/PB5, CS PA4);
                      DIAAGC printed at boot (EV,ENC). DIR_SIGN kept at -1: CHECK IT
                      (flex the arm by hand in IDLE, theta must increase).
                    - VBUS PC3 (x14.33), thermistors on ADC2 by direct registers.
                    - Motor bead: J9 "MOTOR NTC" (PC5) on a 10k PULL-UP (bead to GND)
                      instead of GPIO3/PA2 with a 10k leg to GND; 'PO' = pull-up value.
                      ADC2 shares nothing with the ADC1 current sense, so the reason for
                      the idle-only bead mode is gone: default 'PA' = 2 (read always),
                      i.e. the 40/55 C limits act on the MEASURED temperature while the
                      arm moves. 'PA1' gives back the v2.9t behaviour exactly.
                    - NEW: FET-board NTC RT1 (PC4): coast-stop at 100 C (EV,FETTEMP).
                    - NEW: 'F' gate-driver report ('F1' re-writes it, IDLE only);
                      'S' = stop (same as O0) so the STOP button of ExArMoRehab_v2.9.3 /
                      ExArMoTouch (which send S) can never be ignored.
                    - NEW: motion commands are refused while initFOC / the gate driver
                      is not OK (the MKS build ignored initFOC's result).
                    - LEDs: STAT green blinks in IDLE, solid while driving; FAULT red.
                    - System clock from the 8 MHz crystal (HSI fallback).

  [DV1c, 11 Nov 2026] DEVICE FIXES after the first sessions with the Pi / ExArMo Touch app:
                  - SOFT STOP 'S' (the v2.9z t6 behaviour the Touch/Rehab-2.9.3 STOP buttons
                    expect): from RUN / PASSIVE the arm is lowered slowly to 0° at 'Py' and
                    released at rest — no more drop. 'S' or O0 again = release now. O0 itself
                    is unchanged (instant release, e.g. games "motor free").
                  - MANUAL ZERO 'Z' and 'H1' full auto homing (v2.9z), so the Touch app's
                    "0 here" / "Full auto" buttons work.
                  - HOMING: lift gravity FF from 'PG' (empty device, 0.49 N·m) instead of the
                    session Pg that may include the patient's arm (v2.9.3 lesson); if the lift
                    stalls before 'PR' (2.2 rad ≈ 126°) the push is raised 0.3 A at a time up
                    to 2.5 A (EV,HOMEBOOST) before the stop is accepted; a still-short range
                    is homed with EV,HOMESHORT,<rad>. The Touch app shows both events.
                  - GRAVITY/PLANT DEFAULTS = E13 (24 Oct, full device): g 0.49, phi -0.45,
                    J 0.027, B 0, tau_f 0.20. The apps never send Pf, so with v2.9t's phi = 0
                    the DOB read up to 0.21 N·m of phantom "push down" above ~60°: the
                    "assistive feels resistive above ~90°". A homing stalled short also put
                    the upper virtual wall too low — the HOMING fix above removes that cause.
                  'L' adds Ph= Ghome= g= phi= Rmin_deg= zero=.
  [DV1d, 11 Nov 2026] ASSISTIVE BACK TO v2.9t: the session model defaults return to J 0.015,
                  B 0.05, g 0.60, phi 0, tau_f 0.15 (the old assistive felt better on the
                  device). Homing keeps the E13 gravity via PG 0.49 / PF -0.45 ('PF' new).
                  Soft stop, Z/H1 and the homing boost are unchanged.

  [DV1b, 10 Nov 2026] CURRENT SCALE. Driver V1 reports TRUE amps; the MKS read about 10 % low
                  (Kt R4002 on the same lever: MKS 103.2 vs DV1 93.7 mN·m/A, back-EMF 95.4).
                  Defaults in amps therefore change: P_Ktj 0.760 -> 0.690 N·m/A ('PK', interim
                  = 0.760/1.10, measure with E09 0° on DV1) and P_iq_home 1.2 -> 1.3 A ('Ph',
                  same physical push). Iq limit 6 A / continuous 3 A are kept, i.e. about 10 %
                  more conservative than on the MKS. Everything in N·m (Pg, Kp, Kd, dz, B_v) is
                  physical and unchanged once Ktj is right.

  v2.9t = the ORIGINAL v2.9 (the version that runs best on the new rope) + ONLY two things:
    [R22, 24 Oct 2026] CONSTANTS re-measured after the rope change:
                  N_CAPSTAN 9.05 → 8.68 (kinematics only) and P_Ktj 0.775 → 0.760 N·m/A
                  ('PK'). Nothing else in the control law changed.
    [v2.9t] THERMISTOR — the bead code of v2.9z (= v2.9.2 code, t2 + t3), ported as-is:
                  100k NTC on GPIO3/PA2 with a 10k leg, read at 5 Hz from loop() on ADC3
                  by direct registers (NOT _readRegularADCVoltage(): that uses ADC1 = the
                  motor current sense and gave 5 Hz torque kicks = shaking).
                  While the bead is valid T_est (telemetry field 13, 'L') is the MEASURED
                  temperature; ≥ 40 °C ('PW') EV,TEMPWARN + Iq derate 6 → 3 A,
                  ≥ 55 °C ('PS', 0 = off) EV,TEMP + coast to IDLE until 5 °C cooler
                  (O1/G/H/V refused meanwhile, O0 always accepted).
                  'PA' bead mode: 0 never read (model only) · 1 read ONLY while the motor
                  is off, IDLE/FAULT (default; no ADC activity while it moves, the model
                  continues from the last reading with the same 40/55 °C limits, back in
                  IDLE it snaps to the bead) · 2 always read.
                  No bead / wiring fault → the v2.9 software model takes over, and it now
                  integrates 0 A while the gate driver is off (IDLE/FAULT), so it no longer
                  runs away in IDLE (the 26 000 °C of 18 Oct).
                  'T' dumps the raw volts, 'X<degC>' solves beta, 'PN/PO/PB/PT' bead
                  parameters, 'L' adds T_src/T_mode/T_stop/T_trip.
                  NOT included from v2.9z: manual zero 'Z'/'H1', driver wake-up, soft stop 'S'.

  This is the complete firmware with all test-bench fixes merged:
    [v2.9] Lab constants + lab bug-fixes (16 Oct 2026, for the §4.8 re-run):
      - K_t,j   : the joint torque gain is now ONE measured number, P_Ktj =
                  0.775 N·m per reported ampere ('PK'), from the E9 load-cell
                  ground truth (15 Oct, 6 repeats, ±0.001). It replaces
                  N·Kt = 9.0 × 0.0955 = 0.860 (11 % too high: it ignored the
                  capstan efficiency 83 ± 4 % and used the back-EMF Kt).
                  N_CAPSTAN = 9.05 (E10 hand sweep) is used for kinematics only.
      - R, L    : phase_resistance 0.54 → 0.47 Ω (E01 four-wire Kelvin,
                  R4001 0.472 / R4002 0.496 / R4003 0.485), phase_inductance
                  set explicitly (0.25 mH) and BOTH set BEFORE initFOC so
                  tuneCurrentController() no longer falls back to
                  characteriseMotor() at boot (lab fix v2-E: that injected
                  voltage at boot, overwrote R and was not reproducible).
                  'Pr' / 'Pl' change them live and re-tune.
      - KV_rating 114.1 → 123 (E02, three motors 122.5–123.5 rpm/V).
      - Dead-zone: |τ̂_h| < P_dz ('Pz', default 0.15 N·m) is ignored by the
                  admittance loop. E9 measured a zero-load noise floor of
                  0.018 N·m and a latched drive hysteresis of 0.02–0.13 N·m,
                  so 0.15 N·m (≈ 8σ) sits just above the hysteresis.
      - Thermal : software winding-temperature estimate (first-order model
                  fitted to E05: C_th ≈ 14 J/K, R_th ≈ 7.8 K/W, P_cu =
                  1.5·I²·R). Above 70 °C the Iq limit derates linearly to the
                  continuous rating P_icont ('Pu', 3 A: E05 plateau 77 °C) and
                  holds there — never cuts the motor (a wearer is attached).
                  T_est is telemetry field 13; EV,THERM when derating starts.
                  Default P_iq_lim 8 → 6 A (E05: 6 A reaches 80 °C in 25 s).
      - Vbus guard (lab v2-F): a bad VSENS read no longer caps the phase
                  voltage at 0 V; falls back to 24 V and reports EV,VBUS.
      - Current-PID reset on every IDLE→powered transition (lab v2-G: the
                  q/d PIDs wound up while the gate driver was high-Z).
      - LAB velocity mode uses the tuned velocity PID (P 0.25, I 3, Tf 40 ms,
                  lab GUI g6-C) instead of the SimpleFOC defaults that made
                  the E10 motor-driven sweep vibrate; restored on exit.
      - Telemetry 'D2' (500 Hz) is the recommended logging rate for §4.8/4.9:
                  at 100 Hz the 10× downsampling of the 1 kHz loop was the
                  largest scatter term in the B_v(τ̂) figure.
    [v2-1] DIR_SIGN = -1        : up/down was reversed on the real rig
                                  (pulling up read as "down" in assist/games)
    [v2-2] Passive mode         : new ST_PASSIVE state + 'G<rad>' goto command;
                                  firmware ramps θ_ref at P_pass_speed ('Pv'),
                                  the GUI sequences the sweep (min→max→reps)
    [v2-3] Gentle park          : homing no longer jumps to mid-range and drops
                                  the arm. It ramps slowly to a configurable
                                  park angle P_park ('Pk', set ≈ horizontal /
                                  rest position) and releases only after the
                                  arm has settled there.
    [v2.1] Homing drive fix     : the velocity-servo push chattered on the real
                                  rig (vibrated in place, no motion). Replaced
                                  with a ramped constant-current push; the park
                                  phase now uses the same PD + gravity inner
                                  loop as RUN instead of a velocity servo.
    [v2.2] Homing speed governor: the constant-current push lifted the arm too
                                  fast once free of friction. Push current now
                                  bleeds off above HOME_VCAP (coast-only, never
                                  reverses), giving a gentle ~14°/s approach
                                  with no chatter at stall.
    [v2.8] Lab velocity mode    : 'V<rad/s>' spins the motor shaft under
                                  SimpleFOC velocity control (state 7) for
                                  bench tests (KV back-EMF with a coupled
                                  DUT). 'O0' exits back to torque mode.
    [v2.7] Symmetric assist     : Coulomb friction added to the estimator
                                  model (all three). Un-modeled friction
                                  biased τ̂ upward → B_v asymmetry → moving
                                  down felt resistive in assistive mode.
    [v2.6] True freewheel IDLE  : gate driver high-Z in IDLE/FAULT — an
                                  enabled motor at zero command drags/brakes
                                  when backdriven; now STOP means truly limp.
                                  KV_rating set so 'Pm0' voltage torque mode
                                  is BEMF-compensated and accurate.
    [v2.5] Ultra-smooth passive : PID (integral absorbs stroke-varying
                                  friction, anti-windup), SMOOTH friction FF
                                  (no torque step at reversals), and profile
                                  acceleration feedforward.
    [v2.4] Smooth passive     : trapezoidal (accel-limited) theta_ref profile
                                  ('Pa') + Coulomb friction feedforward ('Pc')
                                  replace the constant-velocity ramp — no more
                                  end-of-sweep kick or low-speed stick-slip.
    [v2.3] Iq-sign auto-cal     : with skip_align the measured q-current
                                  polarity is unverified; wrong polarity
                                  silently corrupts all three estimators.
                                  Homing now correlates commanded vs measured
                                  current and sets P_iq_sign itself (manual
                                  override 'Pq-1' / 'Pq1'; shown in 'L').

  Structure (matches the paper, §3.8):
    Joint model (output side):  J·θ̈ + B·θ̇ + G(θ) = τ_m + τ_h,
                                G(θ) = g·cos(θ+φ),  τ_m = K_t,j·Iq   (K_t,j measured, E9)
    Estimators (all three run in parallel on identical inputs, one "active"):
      [0] Static:  τ̂_h = B·θ̇ + G(θ) − τ_m                (neglects inertia)
      [1] Direct:  τ̂_h = J·θ̈ + B·θ̇ + G(θ) − τ_m          (θ̈ num. diff., LPF'd)
      [2] DOB:     momentum observer, 1st-order Q-filter, bandwidth ω_c
    Admittance:  J_v·θ̈_ref + B_v(τ̂_h)·θ̇_ref = A·τ̂_h + τ_wall
      B_v(τ̂) = B_min + (B_max−B_min)/(1 + (τ̂/τ_0)²)
      A > 0: assist;  A < 0: resistance training.
    Inner loop: joint-space PD on θ_ref + gravity FF → Iq
                (motor stays in foc_current torque mode).

  Homing (two phases, so gravity FF is valid while lifting):
    H → ST_HOME_NEG: ramped constant-current push DOWN to the lower stop,
                     define joint θ=0 there
      → ST_HOME_POS: push UP with gravity feedforward to the upper stop
      → ST_HOME_CENTER: slow θ_ref ramp to P_park tracked by the PD +
                     gravity inner loop; settle; release to IDLE.
    Stall = full push current applied + no progress for STALL_TICKS.
    Phase timeout → FAULT (never pushes forever).
    Homing speed/force is set by 'Ph' (P_iq_home): raise it if the arm
    stalls before the stop, lower it if it hits the stops too hard.

  Serial protocol (Python GUI 'rehab_gui.py', 921600 baud):
    Telemetry (CSV, prefix "D,"):
      D,t_ms,state,theta,theta_ref,vel,iq_meas,tau_m,tau_s,tau_d,tau_dob,Bv,iq_cmd,T_est
    Events (parseable, prefix "EV,"):
      EV,STOP_LOW,...  EV,HOMED,min,max  EV,HOMEFAIL,range  EV,PARKED,theta
      EV,RANGE_FAULT,theta  EV,HOMETIMEOUT  EV,THERM,T,lim (model derate)
      [v2.9t] EV,TEMPOK,T  EV,TEMPFAIL,volts  EV,TEMPWARN,T  EV,TEMP,T (coast-stop)
              EV,TEMPCLEAR,T  EV,TEMPDUMP,...  EV,CALBETA,beta,...
      [DV1]   EV,DRV,ok,...  EV,DRVFAULT,f1,f2,names  EV,DRVREG,...  EV,CSALIGN,code,ga,gb,gc
              EV,ENC,...  EV,FETTEMP,T  EV,FOCFAIL
    Commands (Commander, newline-terminated; parameters WITHOUT space: Pg0.6):
      H        home capstan limits       O0/O1    idle / run (admittance)
      T        [v2.9t] thermistor dump   X<degC>  [v2.9t] solve beta at a known temp
      S        [DV1c] soft stop (lower to 0°, release; S again = now)   F / F1  [DV1] gate driver
      Z        [DV1c] manual zero here   H1       [DV1c] full auto homing
      P G<f>   [DV1c] homing gravity [Nm]   P F<f> [DV1d] homing gravity phase [rad]
      P R<f>   [DV1c] expected range [rad] (HOMEBOOST below it)
      Q<n>     [DV1] encoder check: n raw AS5047P reads + verdict (IDLE/FAULT only)
      E0/E1/E2 estimator static/direct/DOB
      A<f>     assist gain (neg = resistance)
      G<f>     passive goto [rad joint]  (enters ST_PASSIVE from IDLE)
      D<n>     telemetry every n ms (0 = off)
      L        status dump (state, homed, raw angle, limits)
      P g<f> gravity coeff   P f<f> gravity phase   P j<f> model J   P b<f> model B
      P J<f> virtual Jv      P n<f> B_min           P x<f> B_max     P t<f> tau_0
      P w<f> DOB bandwidth   P p<f> pos Kp          P d<f> pos Kd
      P i<f> Iq limit        P h<f> homing Iq
      P v<f> passive speed [rad/s]      P k<f> park angle [rad]
      P y<f> park ramp speed [rad/s]
      [v2.9] P K<f> K_t,j [Nm/A]   P z<f> dead-zone [Nm]   P r<f> R_ph [ohm]
             P l<f> L_ph [H]       P u<f> continuous Iq [A] (thermal derate floor)
      [v2.9t] P W<f> warn °C (derate start, 40)   P S<f> coast-stop °C (55, 0 = off)
             P A<n> bead mode 0/1/2 (1 = idle-only, default)
             P N<f> R25 [ohm]  P O<f> divider [ohm]  P B<f> beta [K]  P T<f> offset [°C]
      M...     standard SimpleFOC motor commands (webcontroller compatible)

  States (telemetry field 2):
    0 IDLE  1 HOME_POS(lift)  2 HOME_NEG(lower)  3 HOME_CENTER(park)
    4 RUN   5 FAULT           6 PASSIVE          7 LAB (velocity)
*/
#include <Arduino.h>
#include <SimpleFOC.h>
#include "current_sense/hardware_specific/stm32/stm32_adc_utils.h"
#include "SimpleFOCDrivers.h"
#include "comms/can/CANCommander.h"
#include "SimpleCANio.h"
#include "exarmo_driverv1.h"          // [DV1] pins, DRV8320S, ADC2, NTCs, clock, LEDs

#define FW_TAG "v2.9t"                // what 'L' reports as fw= (the GUIs check it)
#define FW_FULL "v2.9t-DV1"           // this build
// [DV1] 1 = verify current-sense phase order/polarity at boot (EV,CSALIGN).
#ifndef EXARMO_CS_ALIGN
#define EXARMO_CS_ALIGN 1
#endif

// ======================= pinout: ExArMo Driver V1 (exarmo_driverv1.h) ====
// [DV1] All pins come from the board header. The MKS map does NOT apply here:
// PB12 is nFAULT (was EN_GATE), PA15 is the DRV8320S nSCS (was the AS5047 CS),
// the encoder is on SPI1 (PB3/PB4/PB5, CS PA4), the shunts are PC0/PC1/PC2.
// [DV1] motor thermistor = J9 "MOTOR NTC" (PC5, ADC12_IN15), 10k pull-up R24 to
// +3.3VA, bead to GND. Read on ADC2 (never ADC1 = current sense).
#define THERM_VREF DV1_VDDA

// ======================= mechanism constants ======================
// [R22, 24 Oct 2026] kinematic ratio RE-MEASURED after the larger capstan rope was
// fitted (E10 method A): 8.68. Was 9.05 (13 Oct, old rope). Kinematics only.
static const float N_CAPSTAN = 8.68f;
// [v2.9] joint torque per REPORTED ampere, measured end-to-end on the E9
// load-cell rig. [R22, 24 Oct 2026] re-measured with the new rope: 0.760 Nm/A
// (was 0.775 ± 0.001 on 15 Oct with the old rope; the 18 Oct device log already
// showed 0.761). Use the measured number, not the product N·η·Kt.
// Other motors: Kt per reported amp 0.103–0.107 (±2 %), so one value serves all three.
// [DV1, 10 Nov 2026] 0.760 is in MKS-REPORTED amps. The MKS current sense reads about
// 9-10 % low: Kt of motor R4002 on the same lever was 103.2 mN·m/A on the MKS against
// 93.7 on Driver V1 (ratio 1.101 ± 0.009), and the DV1 value matches back-EMF (95.4)
// within 2 %. Driver V1 reports true amps, so the joint gain in DV1 amps is
// 0.760 / 1.10 ≈ 0.690. INTERIM ESTIMATE: measure K_t,j on Driver V1 (E09 stage 1,
// 0° station) before thesis-grade runs, then put the number here and in the GUI.
// Left at 0.760, the device would over-estimate human torque, under-deliver commanded
// torque and under-compensate gravity by about 10 %.
volatile float P_Ktj = 0.690f;            // [Nm/A] 'PK'  ([DV1] interim, was 0.760 in MKS amps)
// [v2.9] electrical parameters (E01 four-wire Kelvin 13 Oct: R_ph 0.472/0.496/0.485 Ω)
volatile float P_R   = 0.47f;             // [ohm] phase resistance ('Pr')
volatile float P_L   = 0.00025f;          // [H]   phase inductance ('Pl')
static const float KV_MEAS = 123.0f;      // [rpm/V] E02 back-EMF, three motors 122.5–123.5
// [v2-1] flipped after bench test: pulling the arm UP must make θ increase.
// Verify after flashing: flex by hand in IDLE → sidebar angle increases.
static const float DIR_SIGN  = -1.0f;

// ======================= tunable parameters =======================
// --- joint model (identify in E5: RUN with A=0, tune Pg/Pf until arm floats) ---
// [DV1d, 11 Nov 2026] the session model (RUN / assistive, PASSIVE, soft stop) is back to
// the v2.9t values: on the device the old assistive felt better than with the E13 set
// (J 0.027, B 0, g 0.49, phi -0.45, tau_f 0.20) tried in DV1c. Only HOMING keeps the E13
// gravity, through its own PG / PF below.
volatile float P_J      = 0.015f;  // [kg m^2] reflected inertia at joint ('Pj')
volatile float P_B      = 0.05f;   // [Nm s/rad] viscous damping ('Pb')
volatile float P_Gcoef  = 0.60f;   // [Nm] gravity coefficient (supporter + forearm) ('Pg')
volatile float P_Gphase = 0.0f;    // [rad] G = g·cos(theta + phase); set so G peaks at horizontal ('Pf')
// [DV1c] gravity used by HOMING and the park after it, separate from the session model:
// homing is done with nobody in the arm, but the Rehab/Touch apps push a session Pg that
// may include the patient's forearm — too much lift at the bottom, too much pull-down above
// the gravity zero, so the lift stalled short (the v2.9.3 finding). E13 empty-device values.
volatile float P_Ghome  = 0.49f;   // [Nm]  ('PG', E13 empty device)
volatile float P_Ghome_phase = -0.45f;   // [rad] ('PF', E13 phi)
// --- estimators ---
volatile int   est_sel  = 2;       // 0 static, 1 direct, 2 DOB (feeds the admittance)
volatile float P_wc     = 30.0f;   // [rad/s] DOB Q-filter bandwidth (sweep 10..100)
volatile float P_wd     = 150.0f;  // [rad/s] LPF on direct estimator's acceleration
// --- admittance ---
volatile float P_Jv     = 0.02f;   // virtual inertia
volatile float P_Bmin   = 0.05f;   // light (active user)
volatile float P_Bmax   = 0.8f;    // firm (passive user)
volatile float P_tau0   = 0.4f;    // [Nm] effort scale of the B_v(τ̂) transition
volatile float P_assist = 1.0f;    // assist gain A (negative = resistance mode)
// [v2.9] dead-zone on τ̂_h before the admittance law ('Pz'). E9: σ₀ = 0.018 Nm,
// drive hysteresis 0.02–0.13 Nm → 0.15 Nm ignores the latched hysteresis
// without ignoring a real push (a light push is ≈ 0.3–0.5 Nm).
volatile float P_dz     = 0.15f;   // [Nm]
// --- inner position loop (joint space) ---
volatile float P_Kp     = 20.0f;   // [Nm/rad]
volatile float P_Kd     = 0.5f;    // [Nm s/rad]
// --- limits / safety ---
volatile float P_iq_lim   = 6.0f;  // [A] absolute Iq clamp (E05: 6 A → 80 °C in 25 s; thermal derate below)
// [v2.9] software thermal model of the end-winding, fitted to E05 (R4002,
// shaft blocked, still air): C_th ≈ 14 J/K, plateau 77 °C at 3 A (6.4 W) with
// ~27 °C ambient → R_th ≈ 7.8 K/W, τ ≈ 110 s. Predicts 80 °C in 34 s at 6 A
// (measured 25 s) and 52 s at 5 A (measured 53 s). Conservative above 4 A.
volatile float P_icont   = 3.0f;   // [A] continuous rating = derate floor ('Pu')
static const float TH_C   = 14.0f; // [J/K]
static const float TH_RTH = 7.8f;  // [K/W]
static const float TH_AMB = 28.0f; // [°C] assumed ambient (no sensor on the rehab board)
static const float TH_DERATE_LO = 70.0f, TH_DERATE_HI = 80.0f;  // [°C] linear derate band
volatile float T_est = TH_AMB;     // [°C] winding temperature, telemetry field 13 (measured when temp_ok)
// [v2.9t] thermistor (values as in LabTest v2.7.1-R: 100k bead, B 3950, 10k 1 % fixed leg)
volatile float P_th_rfix = 10000.0f;   // [ohm] [DV1] pull-up R24 to +3.3VA   ('PO')
volatile float P_th_r25  = 100000.0f;  // [ohm] NTC nominal at 25 °C         ('PN')
volatile float P_th_beta = 3950.0f;    // [K]   beta — calibrate with X<degC> ('PB')
volatile float P_th_off  = 0.0f;       // [°C]  offset vs a reference         ('PT')
volatile float P_temp_warn = 40.0f;    // [°C]  warn + derate start (logbook §6) ('PW')
volatile float P_temp_stop = 55.0f;    // [°C]  coast-stop, 0 = off (logbook §6)  ('PS')
static const float TEMP_HYST = 5.0f;   // [°C]  must cool this far below the stop to re-arm
volatile float temp_c  = 0.0f;         // [°C]  filtered bead temperature, valid only while temp_ok
volatile bool  temp_ok = false;        // the ONLY thing that says temp_c means anything
volatile bool  temp_tripped = false;   // latched coast-stop
static bool    temp_seeded = false, temp_fault_said = false, temp_warned = false;
static uint32_t temp_last_ms = 0;
volatile bool  ev_temptrip = false, ev_tempfail = false, ev_tempok = false,
               ev_tempwarn = false, ev_tempclear = false;
volatile float ev_t = 0.0f;
static float   iq_lim_thermal = 6.0f;
static bool    derating = false;
// [v2.9t] 'PA' bead mode: 0 = never read (model only)
//                          1 = read ONLY while the motor is off — zero ADC activity
//                              while it moves; the model continues from the last reading
//                          2 = read always
// [DV1] default 2. Mode 1 existed because on the MKS board every bead read could
// disturb the ADC1 current sense (the 5 Hz torque kicks). On Driver V1 the bead is
// read on ADC2, which shares nothing with the injected current conversions on ADC1,
// so the measured temperature can guard the motor while it moves. 'PA1' = v2.9t.
volatile int therm_mode = 2;

// [DV1] board-level state: gate driver, FET thermistor, bus voltage, FOC
volatile bool  drv_fault   = false;    // nFAULT seen low while driving
volatile bool  ev_drvfault = false;
static   bool  drv_cfg_ok  = false;    // DRV8320S holds our settings (checked at 1 Hz)
static   bool  foc_ready   = false;    // initFOC (+ current-sense check) succeeded
static   int   cs_align    = -1;       // EV,CSALIGN code (-1 = not run)
volatile float tfet_c      = NAN;      // [C] FET-board NTC, NAN = open/short
volatile bool  fet_tripped = false;    // latched coast-stop, re-arms 10 C cooler
volatile bool  ev_fettrip  = false;
static uint32_t nfault_ticks = 0;
static float   vbus_meas   = 0.0f;     // [V] boot value (used for PWM scaling)
static float   vbus_now    = 0.0f;     // [V] measured at 5 Hz
// [DV1] 1.2 MKS-reported A = about 1.32 true A: same physical homing push on Driver V1
volatile float P_iq_home  = 1.3f;  // [A] homing/parking servo current budget ('Ph', was 1.2 MKS-A)
// --- [v2-2] passive mode ---
volatile float P_pass_speed = 0.5f;   // [rad/s joint] θ_ref cruise speed ('Pv')
volatile float P_pass_acc   = 1.2f;   // [rad/s²] accel/decel of the profile ('Pa') — lower = softer
volatile float P_fric       = 0.15f;  // [Nm] Coulomb friction feedforward ('Pc') — kills stick-slip ([DV1d] back to v2.9t)
volatile float P_Ki_pass    = 12.0f;  // [Nm/(rad·s)] passive integral gain ('Pe') — absorbs varying friction
static float   pass_integ   = 0.0f;   // integral state (anti-windup clamped)
volatile float passive_target = 0.0f; // [rad joint] current goto target ('G')
// --- [v2.8] lab velocity mode (motor tester: spin an attached DUT) ---
volatile float lab_vel = 0.0f;        // [rad/s MOTOR shaft] 'V' command target
// --- [v2-3] park after homing ---
volatile float P_park       = 0.17f;  // [rad joint] park angle (~10°; 'Pk' — set to horizontal/rest)
volatile float P_park_speed = 0.4f;   // [rad/s] park ramp speed ('Py')

// --- [v2.3] Iq measurement sign ---
// With current_sense.skip_align=true the polarity of the measured q-current is
// unverified; an inverted sign silently corrupts all three estimators
// (τ_m enters with the wrong sign → τ̂ ≈ 2G bias). Auto-calibrated during
// homing by correlating commanded vs measured current; manual override 'Pq'.
volatile float P_iq_sign = 1.0f;
static float    iqcal_acc = 0;
static uint32_t iqcal_n   = 0;

// --- homing behaviour ---
static const uint32_t HOME_RAMP_TICKS = 800; // [ms] ramp-in time of the push current
static const float HOME_VCAP     = 0.25f;  // [rad/s joint] homing speed cap (~14°/s; push coasts above it)
static const float HOME_MARGIN   = 0.05f;  // [rad joint] soft margin inside hard stops
static const float PROGRESS_EPS  = 0.03f;  // [rad joint] progress that resets the stall clock
static const uint32_t STALL_TICKS   = 400;   // [ms] saturated + no progress → hard stop
static const uint32_t HOME_TIMEOUT  = 25000; // [ms] per-phase timeout → FAULT
static const uint32_t PARK_TIMEOUT  = 8000;  // [ms] park phase always releases
static const float MIN_RANGE     = 0.30f;  // [rad] smaller found range = homing failed
// [DV1c] adaptive lift: a stall BEFORE P_range_min is friction, not the upper stop → the
// push is raised in HOME_BOOST_STEP steps (EV,HOMEBOOST) up to HOME_BOOST_MAX; if it is
// still short the device is homed anyway and says EV,HOMESHORT,<range> (after EV,HOMED).
volatile float P_range_min = 2.2f;          // [rad] 'PR' (≈126°; mechanism ≈140°) — set 5–10° below the real range
static const float HOME_BOOST_STEP = 0.3f;  // [A]
static const float HOME_BOOST_MAX  = 2.5f;  // [A] (never above the Iq limit)
static float home_push = 0.0f;              // [A] push budget of the current lift
volatile bool  ev_homeboost = false, ev_homeshort = false;
volatile float ev_hb_a = 0, ev_hb_b = 0, ev_hs = 0;
// [DV1c] soft stop 'S' (v2.9z t6): RUN/PASSIVE → lower to 0° with the park controller, release
volatile float park_goal = -1.0f;           // [rad] <0 → park after homing (P_park), else soft-stop target
static uint32_t park_timeout = PARK_TIMEOUT;// [ms] distance-based for a soft stop
volatile bool  ev_softstop_done = false;
// [DV1c] manual zero 'Z' (v2.9z)
bool  zero_manual = false;        // joint 0 set by hand with 'Z'
bool  zero_set    = false;        // any valid zero (manual or found by homing)
static bool iqcal_pending = false;// lift-only homing decides the Iq polarity on the lift
static const float WALL_K        = 30.0f;  // [Nm/rad] virtual wall stiffness
static const float WALL_D        = 1.0f;   // [Nm s/rad] virtual wall damping

// ======================= control state ============================
enum State : uint8_t { ST_IDLE=0, ST_HOME_POS, ST_HOME_NEG, ST_HOME_CENTER,
                       ST_RUN, ST_FAULT, ST_PASSIVE, ST_LAB };    // [v2-2, v2.8]
volatile State state = ST_IDLE;

float home_motor_offset = 0;      // motor angle [rad] that maps to joint θ = 0 (lower stop)
float lim_min = 0, lim_max = 0;   // joint-space working limits [rad] (valid after homing)
bool  homed = false;

// live signals (written in ISR context, read in loop for telemetry)
volatile float s_theta=0, s_vel=0, s_theta_ref=0, s_vel_ref=0;
volatile float s_tau_m=0, s_tau_s=0, s_tau_d=0, s_tau_dob=0, s_Bv=0, s_iq_cmd=0;

// ======================= SimpleFOC objects ========================
BLDCMotor motor = BLDCMotor(7);
// [DV1] DRV8320S in 3x PWM mode: INHx = PWM (TIM1 CH1..3), INLx = phase enable.
// driver.disable() = INLx low = all FETs off (true freewheel, [v2.6]) without
// touching ENABLE (sleep would put the DRV8320S settings at risk). -D EXARMO_PWM6=1 = 6x.
#if EXARMO_PWM6
BLDCDriver6PWM driver = BLDCDriver6PWM(DV1_INHA,DV1_INLA, DV1_INHB,DV1_INLB, DV1_INHC,DV1_INLC);
#else
BLDCDriver3PWM driver = BLDCDriver3PWM(DV1_INHA, DV1_INHB, DV1_INHC, DV1_INLA, DV1_INLB, DV1_INLC);
#endif
// [DV1] inline INA240A1, 3 mOhm x 20 = 60 mV/A, all three phases, sampled by the
// PWM-synchronised injected ADC path of SimpleFOC's LowsideCurrentSense.
LowsideCurrentSense current_sense = LowsideCurrentSense(DV1_SHUNT_OHM, DV1_CSA_GAIN, DV1_ISENSE_A, DV1_ISENSE_B, DV1_ISENSE_C);
MagneticSensorSPI sensor = MagneticSensorSPI(AS5047_SPI, DV1_ENC_CS, 10000000);
SPIClass SPI_1(DV1_ENC_MOSI, DV1_ENC_MISO, DV1_ENC_SCK);   // [DV1] encoder, J11
SPIClass SPI_3(DV1_DRV_MOSI, DV1_DRV_MISO, DV1_DRV_SCK);   // [DV1] DRV8320S only
Commander command = Commander(Serial);
CANio can(DV1_CAN_RX, DV1_CAN_TX);
HardwareTimer* ctrl_timer = nullptr;   // [DV1] so 'Q' can pause the FOC ISR for a raw encoder read
CANCommander commandc(can, 15);

// ======================= control loop =============================
static const float FOC_HZ  = 20000.0f;
static const int   CTRL_DIV = 20;             // control loop = 1 kHz
static const float DT = CTRL_DIV / FOC_HZ;    // 1 ms

// estimator internal state
static float dob_integral = 0, dob_r = 0;
static float vel_prev = 0, acc_filt = 0;

// homing internal state — all timing in control ticks (1 ms each), ISR-safe
static uint32_t tick = 0;                  // increments every controlUpdate (1 kHz)
static uint32_t state_tick = 0;            // tick when the current state was entered
static float    progress_ref = 0;          // last joint angle with forward progress
static uint32_t progress_tick = 0;         // tick of that progress
static State    last_state = ST_IDLE;
// event codes set in ISR, printed from loop() (Serial in ISR is unsafe)
// 1 lower stop, 2 homed(min,max), 3 homing failed(range), 4 parked(theta),
// 5 range fault(theta), 6 homing timeout
volatile uint8_t evt = 0;
volatile float   evt_a = 0, evt_b = 0;

static inline float clampf(float v, float lo, float hi){ return v<lo?lo:(v>hi?hi:v); }

// gravity torque at joint angle theta
static inline float gravity(float theta){ return P_Gcoef * cosf(theta + P_Gphase); }
static inline float gravity_home(float theta){ return P_Ghome * cosf(theta + P_Ghome_phase); }   // [DV1c/d]

void controlUpdate(){
  // ---- joint-space kinematics from motor-side sensor ----
  float theta = DIR_SIGN * (motor.shaft_angle    - home_motor_offset) / N_CAPSTAN;
  float vel   = DIR_SIGN *  motor.shaft_velocity / N_CAPSTAN;

  // ---- measured motor torque at the joint (sign-corrected measured Iq) ----
  float iq_meas = P_iq_sign * motor.current.q;           // [v2.3] polarity fixed by homing auto-cal
  float tau_m   = DIR_SIGN * P_Ktj * iq_meas;              // [v2.9] measured joint gain

  // [v2.9] thermal estimate from the MEASURED current (1 kHz Euler step)
  // [v2.9t] with a valid bead T_est IS the measured temperature and the rehab
  // band (warn → stop) replaces the model's 70–80 °C band. Without a bead the
  // model runs, but with 0 A while the gate driver is off (the DRV8301 sense
  // amps are off then and the current reads garbage — the v2.9.1 fix).
  // [DV1] the INA240s read a true 0 A with the bridge off; the 0 A rule is kept
  // because it is also physically right (no copper loss with the FETs off).
  {
    bool  drv_on = (state != ST_IDLE && state != ST_FAULT);
    float lo = TH_DERATE_LO, hi = TH_DERATE_HI;
    bool bead_paused = (therm_mode == 1 && drv_on);   // bead not read while running
    if (temp_ok && !bead_paused){
      T_est = temp_c;                                   // measured
      lo = P_temp_warn;
      hi = (P_temp_stop > P_temp_warn + 1.0f) ? P_temp_stop : P_temp_warn + 1.0f;
    } else if (temp_ok && bead_paused){
      // running: model continues from the last MEASURED value, rehab limits kept
      float ith = fminf(fmaxf(fabsf(iq_meas), fabsf(s_iq_cmd)), 1.5f * P_iq_lim);
      float pcu = 1.5f * ith * ith * P_R;
      T_est += ((pcu - (T_est - TH_AMB) / TH_RTH) / TH_C) * DT;
      if (!(T_est > 0.0f && T_est < 250.0f)) T_est = temp_c;
      lo = P_temp_warn;
      hi = (P_temp_stop > P_temp_warn + 1.0f) ? P_temp_stop : P_temp_warn + 1.0f;
      if (P_temp_stop > 0.0f && !temp_tripped && T_est >= P_temp_stop){   // same coast-stop
        temp_tripped = true; state = ST_IDLE; ev_temptrip = true; ev_t = T_est;
      }
    } else {
      float ith = drv_on ? fminf(fmaxf(fabsf(iq_meas), fabsf(s_iq_cmd)), 1.5f * P_iq_lim) : 0.0f;
      float pcu = 1.5f * ith * ith * P_R;
      T_est += ((pcu - (T_est - TH_AMB) / TH_RTH) / TH_C) * DT;
      if (!(T_est > 0.0f && T_est < 250.0f)) T_est = TH_AMB;   // NaN / runaway guard
    }
    // coast-stop latch enforced every tick: IDLE = gate driver high-Z, arm free
    // [DV1] the FET-board NTC latch uses the same net
    if ((temp_tripped || fet_tripped) && state != ST_IDLE && state != ST_FAULT) state = ST_IDLE;
    // [DV1] a gate-driver fault keeps the board in FAULT until a new command clears it
    if (drv_fault && state != ST_IDLE && state != ST_FAULT) state = ST_FAULT;
    float f = clampf((T_est - lo) / (hi - lo), 0.0f, 1.0f);
    float lim = P_iq_lim + f * (fminf(P_icont, P_iq_lim) - P_iq_lim);
    iq_lim_thermal = lim;
    bool d = f > 0.0f;
    if (d != derating){ derating = d; evt = 7; evt_a = T_est; evt_b = lim; }
  }

  float G = gravity(theta);
  // [v2.7] Coulomb friction MODEL term (smooth sign, same P_fric as passive).
  // Without it, friction leaks into τ̂ as phantom effort: moving up it adds
  // to your effort (B_v drops, feels light), moving down it cancels part of
  // it (B_v stays high, feels like resistance) — the down-feels-heavy bug.
  float fricm = P_fric * clampf(vel / 0.05f, -1.0f, 1.0f);

  // ================= three estimators (always computed) =================
  // [0] static — neglects inertia
  float tau_s = P_B*vel + G + fricm - tau_m;
  // [1] direct — numerical differentiation of velocity, light LPF
  float acc_raw = (vel - vel_prev) / DT;
  vel_prev = vel;
  float a = P_wd*DT/(1.0f + P_wd*DT);                    // 1st-order LPF coefficient
  acc_filt += a * (acc_raw - acc_filt);
  float tau_d = P_J*acc_filt + P_B*vel + G + fricm - tau_m;
  // [2] DOB — momentum observer, differentiation-free, cutoff P_wc
  //   integ' = τ_m − B·θ̇ − G − fric + r ;  r = ω_c(J·θ̇ − integ) ⇒ r → τ̂_h
  dob_integral += (tau_m - P_B*vel - G - fricm + dob_r) * DT;
  dob_r = P_wc * (P_J*vel - dob_integral);
  float tau_dob = dob_r;

  float tau_h = (est_sel==0) ? tau_s : (est_sel==1) ? tau_d : tau_dob;

  // ================= state machine =================
  float iq_cmd = 0;
  tick++;                                   // 1 kHz → 1 tick = 1 ms

  // state-entry bookkeeping
  if (state != last_state){
    last_state = state; state_tick = tick;
    progress_ref = theta; progress_tick = tick;
    if (state == ST_HOME_POS) home_push = P_iq_home;   // [DV1c] every lift starts at Ph
  }

  switch(state){
    // -------- homing phase 1 (ST_HOME_NEG, dir −1): push DOWN to lower stop
    // -------- homing phase 2 (ST_HOME_POS, dir +1): push UP with gravity FF
    case ST_HOME_POS:
    case ST_HOME_NEG: {
      bool  lifting = (state==ST_HOME_POS);
      float dir = lifting ? +1.0f : -1.0f;
      // [v2.1] ramped CONSTANT-CURRENT push (the proven method).
      // The velocity-servo variant chattered on the real rig: its gain
      // (P_iq_home/HOME_V = 8 A per rad/s) + capstan stiction made the arm
      // vibrate in place instead of moving. A slowly ramped push has no
      // feedback loop to oscillate — it just leans on the arm until it moves.
      float frac = clampf((tick - state_tick) / (HOME_RAMP_TICKS + 1.0f), 0.0f, 1.0f);
      // [DV1c] lift push budget = Ph, raised in steps by the stall logic below
      if (lifting && home_push < P_iq_home) home_push = P_iq_home;
      float push_lvl = lifting ? home_push : P_iq_home;
      float iq_push = dir * push_lvl * frac;
      // [v2.2] one-sided speed governor: constant current has no speed
      // regulation, so after breaking free the arm accelerated all the way to
      // the stop. Above HOME_VCAP the push current bleeds off linearly and is
      // fully removed by ~1.5×HOME_VCAP. It only ever REDUCES the push toward
      // zero (coast) — never reverses — and it is inactive at stall, so the
      // stick-slip chatter of the old velocity servo cannot return.
      float over = dir*vel - HOME_VCAP;
      if (over > 0.0f) iq_push -= dir * (push_lvl / (0.5f*HOME_VCAP)) * over;
      if (dir > 0) iq_push = clampf(iq_push, 0.0f, push_lvl);
      else         iq_push = clampf(iq_push, -push_lvl, 0.0f);
      // gravity feedforward (valid in phase 2: joint zero was set at the lower
      // stop). Carries the arm's weight so the push budget is not consumed by
      // gravity — this is what lets homing lift with the arm support attached.
      // [DV1c] homing gravity = the EMPTY device (PG), not the session Pg (v2.9.3 lesson)
      float iq_grav = lifting ? gravity_home(theta)/P_Ktj : 0.0f;
      iq_cmd = DIR_SIGN * clampf(iq_push + iq_grav, -P_iq_lim, P_iq_lim);
      // stall detection only counts once the full push current is applied
      // (gravity FF excluded, so carrying the arm's weight is not mistaken
      // for pushing on a stop)
      bool pushing = frac > 0.95f;
      // [v2.3] Iq-sign calibration: while pushing with a known commanded
      // current, correlate it with the raw measurement
      if (frac > 0.9f){ iqcal_acc += iq_cmd * motor.current.q; iqcal_n++; }
      if (dir*(theta - progress_ref) > PROGRESS_EPS){ progress_ref = theta; progress_tick = tick; }
      if (!pushing) progress_tick = tick;    // only accumulate stall time while pushing hard
      if (tick - progress_tick > STALL_TICKS){
        if (!lifting){
          // lower stop found → define joint zero here; gravity model becomes valid
          home_motor_offset = motor.shaft_angle;
          zero_set = true; zero_manual = false;   // [DV1c] automatic zero
          // [v2.3] decide measurement polarity from the correlation
          if (iqcal_n > 300){
            P_iq_sign = (iqcal_acc >= 0.0f) ? 1.0f : -1.0f;
          }
          evt = 1; evt_a = P_iq_sign;
          state = ST_HOME_POS;
        } else {
          float range = theta;               // upper stop, already in the zeroed joint frame
          float boost_cap = fminf(fmaxf(HOME_BOOST_MAX, P_iq_home), P_iq_lim);
          if (range < P_range_min && home_push < boost_cap - 1e-3f){
            // [DV1c] stalled before a plausible top → rope friction, not the stop: push harder
            home_push = fminf(home_push + HOME_BOOST_STEP, boost_cap);
            progress_ref = theta; progress_tick = tick;
            ev_homeboost = true; ev_hb_a = range; ev_hb_b = home_push;
          } else {
          // [DV1c] down phase skipped (manual zero) → decide the Iq polarity from the lift
          if (iqcal_n > 300 && iqcal_pending){
            P_iq_sign = (iqcal_acc >= 0.0f) ? 1.0f : -1.0f;
          }
          iqcal_pending = false;
          if (range < MIN_RANGE){            // stalled too early → model/current insufficient
            evt = 3; evt_a = range;
            state = ST_FAULT;
          } else {
            lim_min = HOME_MARGIN;
            lim_max = range - HOME_MARGIN;
            homed = true;
            evt = 2; evt_a = lim_min; evt_b = lim_max;
            if (range < P_range_min){ ev_homeshort = true; ev_hs = range; }   // [DV1c] after EV,HOMED
            state = ST_HOME_CENTER;
            park_goal = -1.0f;               // [DV1c] park after homing → P_park
            park_timeout = PARK_TIMEOUT;
            s_theta_ref = theta;             // [v2-3] bumpless: park ramp starts HERE
            s_vel_ref = 0;
          }
          }
        }
      }
      // never push forever: phase timeout → motor off + message
      if (tick - state_tick > HOME_TIMEOUT){ evt = 6; state = ST_FAULT; }
      break;
    }

    // -------- [v2-3] park: slow ramp of θ_ref to P_park, settle, release ----
    case ST_HOME_CENTER: {
      // [DV1c] park_goal >= 0 → soft stop to that angle (session gravity, patient attached);
      //        else the park after homing (empty device → PG)
      bool soft = (park_goal >= 0.0f);
      float target = clampf(soft ? park_goal : P_park, lim_min, lim_max);
      // ramp the REFERENCE (no jump), then velocity-limited servo tracks it
      float d = target - s_theta_ref;
      float step = P_park_speed * DT;
      if      (d >  step) s_theta_ref += step;
      else if (d < -step) s_theta_ref -= step;
      else                s_theta_ref  = target;
      s_vel_ref = clampf(d / DT, -P_park_speed, P_park_speed);
      // [v2.1] track the ramp with the SAME PD + gravity FF inner loop as
      // RUN/PASSIVE (well damped) — not a raw velocity servo, which chatters
      float Gp = soft ? G : gravity_home(theta);                 // [DV1c]
      float tau_cmd = P_Kp*(s_theta_ref - theta) + P_Kd*(s_vel_ref - vel) + Gp;
      iq_cmd = DIR_SIGN * clampf(tau_cmd/P_Ktj, -P_iq_lim, P_iq_lim);
      // release only once the arm has SETTLED at the park position (no free-fall:
      // P_park should be the rest/horizontal position, where releasing is safe)
      bool arrived = (fabsf(target - theta) < 0.03f && fabsf(vel) < 0.05f);
      if (arrived || tick - state_tick > park_timeout){
        evt = 4; evt_a = theta;
        if (soft) ev_softstop_done = true;   // [DV1c]
        park_goal = -1.0f; park_timeout = PARK_TIMEOUT;
        state = ST_IDLE;                     // guaranteed exit: never holds forever
      }
      break;
    }

    // -------- [v2-2, v2.4, v2.5] passive: trapezoid + PID + feedforwards ----
    case ST_PASSIVE: {
      if(!homed){ state = ST_IDLE; break; }
      float target = clampf(passive_target, lim_min, lim_max);
      // online trapezoidal profile (v2.4)
      float d = target - s_theta_ref;
      float dirn = (d >= 0.0f) ? 1.0f : -1.0f;
      float v_stop = sqrtf(2.0f * P_pass_acc * fabsf(d));
      float v_des  = dirn * fminf(P_pass_speed, v_stop);
      float dv = clampf(v_des - s_vel_ref, -P_pass_acc*DT, P_pass_acc*DT);
      float a_ref = dv / DT;                                // profile acceleration
      s_vel_ref += dv;
      s_theta_ref += s_vel_ref * DT;
      if (fabsf(target - s_theta_ref) < 1e-3f && fabsf(s_vel_ref) < 5e-3f){
        s_theta_ref = target; s_vel_ref = 0.0f;
      }
      s_theta_ref = clampf(s_theta_ref, lim_min, lim_max);
      // [v2.5] SMOOTH friction feedforward: proportional ramp through zero
      // velocity instead of an instant sign flip — no torque step (clunk)
      // at the sweep reversals.
      float fric = P_fric * clampf(s_vel_ref / 0.05f, -1.0f, 1.0f);
      // [v2.5] integral action: absorbs friction that VARIES along the
      // stroke (drum eccentricity, rope tension) plus gravity-model error,
      // which pure PD leaves as a wobbling lag. Anti-windup clamped.
      float e = s_theta_ref - theta;
      pass_integ = clampf(pass_integ + P_Ki_pass * e * DT, -0.5f, 0.5f);
      // [v2.5] acceleration feedforward so the PD doesn't have to generate
      // the profile's accel torque reactively.
      float tau_cmd = P_Kp*e + P_Kd*(s_vel_ref - vel) + pass_integ
                    + G + fric + P_J * a_ref;
      iq_cmd = DIR_SIGN * tau_cmd / P_Ktj;
      // same safety as RUN
      if (homed && (theta < lim_min - 0.15f || theta > lim_max + 0.15f)){
        evt = 5; evt_a = theta; state = ST_FAULT;
      }
      break;
    }

    case ST_RUN: {
      if(!homed){ state = ST_IDLE; break; }
      // [v2.9] dead-zone: inside ±P_dz the estimate is treated as zero effort
      // (noise floor + latched drive hysteresis, E9), outside it is shifted
      // so there is no step at the edge.
      float tau_hz = (tau_h >  P_dz) ? tau_h - P_dz :
                     (tau_h < -P_dz) ? tau_h + P_dz : 0.0f;
      // ---- effort-adaptive virtual damping ----
      float x = tau_hz / P_tau0;
      float Bv = P_Bmin + (P_Bmax - P_Bmin) / (1.0f + x*x);
      s_Bv = Bv;
      // ---- virtual walls near the capstan limits ----
      float tau_wall = 0;
      if (s_theta_ref < lim_min) tau_wall = WALL_K*(lim_min - s_theta_ref) - WALL_D*s_vel_ref;
      if (s_theta_ref > lim_max) tau_wall = -WALL_K*(s_theta_ref - lim_max) - WALL_D*s_vel_ref;
      // ---- admittance dynamics: J_v·θ̈_ref = A·τ̂_h − B_v·θ̇_ref + τ_wall ----
      float acc_ref = (P_assist*tau_hz - Bv*s_vel_ref + tau_wall) / P_Jv;
      s_vel_ref   += acc_ref * DT;
      s_theta_ref += s_vel_ref * DT;
      // hard clamp just outside the walls
      s_theta_ref = clampf(s_theta_ref, lim_min - 0.03f, lim_max + 0.03f);
      // ---- inner joint-space PD + gravity feedforward → Iq ----
      float tau_cmd = P_Kp*(s_theta_ref - theta) + P_Kd*(s_vel_ref - vel) + G;
      iq_cmd = DIR_SIGN * tau_cmd / P_Ktj;
      // safety: measured angle escaped the mechanical range → fault
      if (homed && (theta < lim_min - 0.15f || theta > lim_max + 0.15f)){
        evt = 5; evt_a = theta; state = ST_FAULT;
      }
      break;
    }

    case ST_LAB:                        // [v2.8] SimpleFOC velocity loop
      break;                            // (move() routed below)

    case ST_FAULT: iq_cmd = 0; break;
    case ST_IDLE: default:
      iq_cmd = 0;                       // fully limp: user can move the arm freely
      s_theta_ref = theta; s_vel_ref = 0;  // bumpless transfer into RUN / PASSIVE
      break;
  }

  if (state == ST_LAB){
    motor.move(lab_vel);                // [v2.8] velocity target [rad/s motor]
    iq_cmd = 0;
  } else {
    float lim = fminf(P_iq_lim, iq_lim_thermal);              // [v2.9] thermal derate
    iq_cmd = clampf(iq_cmd, -lim, lim);
    motor.move(iq_cmd);
  }

  // [v2.6] TRUE freewheel in IDLE/FAULT: an enabled motor at zero command is
  // NOT free — foc_current chases a (possibly offset) measured zero, and
  // voltage mode at 0 V dynamically brakes the BEMF (~0.6 Nm drag at 1 rad/s
  // joint on this rig). Gate driver goes high-Z instead; loopFOC keeps
  // running so the encoder/telemetry stay live for the GUI and games.
  bool want_power = (state != ST_IDLE && state != ST_FAULT);
  static bool powered = true;
  if (want_power != powered){
    powered = want_power;
    if (powered){
      motor.PID_current_q.reset();        // [v2.9] lab fix v2-G: the q/d PIDs wound up
      motor.PID_current_d.reset();        //        against the high-Z driver
      driver.enable();
    }
    else         driver.disable();
  }

  // [DV1] gate-driver fault: nFAULT low while the bridge drives. The DRV8320S has
  // already turned its FETs off (latched OCP / UVLO / GDF / OTSD); FAULT makes the
  // firmware agree (driver.disable() next tick, arm free) and loop() reports it.
  // Two consecutive ticks so one noisy sample on the open-drain line cannot trip it.
  if (powered && dv1_nfault_low()){
    if (++nfault_ticks >= 2 && !drv_fault){ drv_fault = true; ev_drvfault = true; state = ST_FAULT; }
  } else {
    nfault_ticks = 0;
  }

  // publish for telemetry
  s_theta = theta; s_vel = vel;
  s_tau_m = tau_m; s_tau_s = tau_s; s_tau_d = tau_d; s_tau_dob = tau_dob;
  s_iq_cmd = iq_cmd;
}

// ======================= [v2.9t] thermistor =======================
// Ported from v2.9z (= v2.9.2 bead code + t2 + t3). Runs from loop() at 5 Hz —
// NEVER from the 1 kHz ISR.
// [DV1] Driver V1: bead on J9 (PC5), LOW side of a 10k pull-up to +3.3VA:
//   ratio = counts/4095 = R_ntc/(R_ntc + R_pull)  →  R_ntc = R_pull·ratio/(1 − ratio)
//   1/T = 1/298.15 + ln(R_ntc/R25)/beta
// Ratiometric: the pull-up and the ADC reference are the same +3.3VA rail.
// Both rails are rejected: an open lead / empty J9 (pull-up → 3.3 V) reads
// "very cold", a short (0 V) "very hot", and neither is a temperature.
// The bead is read on ADC2 by direct registers, NOT with _readRegularADCVoltage().
// That helper runs a regular conversion on ADC1 — the ADC that samples the motor
// phase currents (injected). On the MKS board every such read (5×/s) could corrupt a
// current sample → a small torque kick 5×/s = the "shaking" of v2.9.2 / v2.9.3e.
// v2.9t moved PA2 to ADC3; PC5 exists only on ADC1/ADC2, so Driver V1 uses ADC2:
// ~25 µs, no shared state with ADC1 (see dv1_adc2_ratio in exarmo_driverv1.h).
static float therm_ratio(){ return dv1_adc2_ratio_avg(DV1_TMOT_CH, 4); }   // 0..1, −1 = timeout
static float therm_volts(float ratio){ return (ratio >= 0.0f) ? ratio * THERM_VREF : ratio; }

static void thermUpdate(){
  if (therm_mode == 0){ temp_ok = false; temp_seeded = false; return; }   // model only
  // mode 1: motor powered → don't touch any ADC; keep temp_ok/temp_c as last read
  if (therm_mode == 1 && state != ST_IDLE && state != ST_FAULT) return;
  float ratio = therm_ratio();                       // [DV1] J9 on ADC2
  float v = therm_volts(ratio);                      // volts, for the messages
  bool good = dv1_ntc_ratio_ok(ratio);
  float t = 0.0f;
  if (good){
    float r_ntc = dv1_ntc_r(ratio, P_th_rfix);
    if (r_ntc > 1.0f && isfinite(r_ntc)){
      float inv = 1.0f / 298.15f + logf(r_ntc / P_th_r25) / P_th_beta;
      t = (inv > 1e-6f) ? (1.0f / inv - 273.15f + P_th_off) : 0.0f;
      good = isfinite(t) && t > -40.0f && t < 200.0f;
    } else good = false;
  }
  if (!good){
    if (!temp_fault_said){ temp_fault_said = true; ev_tempfail = true; ev_t = v; }
    temp_ok = false; temp_seeded = false; temp_c = 0.0f;
    // the latch is kept: a bead that failed while hot must not unlock the motor
    return;
  }
  temp_fault_said = false;
  if (!temp_seeded){ temp_c = t; temp_seeded = true; ev_tempok = true; ev_t = t; }
  else             { temp_c += 0.25f * (t - temp_c); }    // ~0.8 s at 5 Hz
  temp_ok = true;

  // warning (once per crossing, 2 °C hysteresis)
  if (!temp_warned && temp_c >= P_temp_warn){ temp_warned = true; ev_tempwarn = true; ev_t = temp_c; }
  else if (temp_warned && temp_c < P_temp_warn - 2.0f) temp_warned = false;

  // coast-stop, latched with hysteresis
  if (P_temp_stop > 0.0f){
    if (!temp_tripped && temp_c >= P_temp_stop){
      temp_tripped = true; state = ST_IDLE;
      ev_temptrip = true; ev_t = temp_c;
    } else if (temp_tripped && temp_c < P_temp_stop - TEMP_HYST){
      temp_tripped = false; ev_tempclear = true; ev_t = temp_c;
    }
  } else if (temp_tripped){ temp_tripped = false; ev_tempclear = true; ev_t = temp_c; }
}

// [DV1] FET-board thermistor RT1 (10k B3950 on a 10k pull-up, PC4), 5 Hz from
// loop(). Protects the BOARD; same coast-stop as the motor bead (IDLE = arm free).
// An open/shorted RT1 shows as T_fet=nan in 'L' but does not stop the device.
static void fetUpdate(){
  float t = dv1_read_tfet();
  tfet_c = t;
  if (!isfinite(t)) return;
  if (!fet_tripped && t >= DV1_TFET_TRIP){
    fet_tripped = true; state = ST_IDLE; ev_fettrip = true;
  } else if (fet_tripped && t < DV1_TFET_TRIP - DV1_TFET_HYST){
    fet_tripped = false;
  }
}

// [DV1] Bring the gate driver back if it lost its settings or latched a fault.
// Called from motion commands (loop context; SPI3 is not used by the ISR). A
// latched DRV8320S fault is cleared by the NEXT motion command — if the cause is
// still there it latches again within microseconds with the FETs already off.
static bool drvRecover(){
  bool was_fault = drv_fault || dv1_nfault_low();
  dv1_drv_read_all();
  uint16_t f1 = dv1drv.fault1, f2 = dv1drv.fault2;
  drv_cfg_ok = dv1_drv_configure();               // re-write + CLR_FLT + verify
  delayMicroseconds(50);
  if (drv_cfg_ok && !dv1_nfault_low()){
    if (was_fault){
      char txt[96]; dv1_drv_fault_text(f1, f2, txt, sizeof(txt));
      Serial.print(F("EV,MSG,gate-driver fault cleared (was: ")); Serial.print(txt); Serial.println(')');
    }
    drv_fault = false;
    return true;
  }
  return false;
}
static bool hwBlocked(){
  if (fet_tripped){
    Serial.print(F("EV,FETTEMP,")); Serial.println((float)tfet_c, 1);
    Serial.print(F("EV,MSG,power stage too hot - cool below "));
    Serial.print(DV1_TFET_TRIP - DV1_TFET_HYST, 0); Serial.println(F(" C (FET NTC; O0 is always accepted)"));
    return true;
  }
  if (!(drv_cfg_ok && !drv_fault && !dv1_nfault_low()) && !drvRecover()){
    dv1_drv_read_all();
    char txt[96]; dv1_drv_fault_text(dv1drv.fault1, dv1drv.fault2, txt, sizeof(txt));
    Serial.print(F("EV,DRVFAULT,0x")); Serial.print(dv1drv.fault1, HEX);
    Serial.print(F(",0x")); Serial.print(dv1drv.fault2, HEX);
    Serial.print(','); Serial.println(txt);
    Serial.print(F("EV,MSG,gate driver not ready (VBUS=")); Serial.print(vbus_now, 1);
    Serial.println(F(" V) - 24 V connected? 'F' shows the registers"));
    return true;
  }
  if (!foc_ready){
    Serial.println(F("EV,FOCFAIL,motor not initialised (encoder / current sense / 24 V at boot) - fix it and press RESET"));
    return true;
  }
  return false;
}

// refuse new motion while the coast-stop latch is set
static bool tempBlocked(){
  if (hwBlocked()) return true;             // [DV1] gate driver / FET temperature / initFOC
  if (!temp_tripped) return false;
  Serial.print(F("EV,TEMP,")); Serial.println(temp_c, 1);
  Serial.print(F("EV,MSG,motor too hot - cool below "));
  Serial.print(P_temp_stop - TEMP_HYST, 1); Serial.println(F(" C (O0 is always accepted)"));
  return true;
}

// ======================= commander callbacks ======================
void doMotor(char* cmd){ command.motor(&motor, cmd); }

// [DV1] leave lab velocity mode whenever we are not IN it any more. v2.9t only
// restored torque control on O0 sent while state == ST_LAB; a coast-stop / fault
// that moved ST_LAB -> IDLE/FAULT inside the ISR left motor.controller = velocity,
// and a following H / O1 / G would have fed a torque command to the velocity loop.
static void leaveLab(){
  if (state == ST_LAB) return;
  if (motor.controller == MotionControlType::velocity){
    lab_vel = 0.0f;
    motor.controller = MotionControlType::torque;
    motor.LPF_velocity.Tf = 0.005f;        // [v2.9] the estimator's velocity filter
  }
}

void doHome (char* cmd){
  if (tempBlocked()) return;                // [v2.9t]
  leaveLab();                               // [DV1]
  if(state==ST_IDLE || state==ST_FAULT){
    bool full = (atoi(cmd) == 1);        // [DV1c, v2.9z] 'H1' = always full automatic homing
    park_goal = -1.0f; park_timeout = PARK_TIMEOUT;
    homed = false;
    iqcal_acc = 0; iqcal_n = 0;          // [v2.3] fresh Iq-sign calibration
    if (zero_manual && !full){
      // [DV1c, v2.9z] joint 0 was set by hand ('Z') → skip the push-down, lift from here
      iqcal_pending = true;
      Serial.println(F("EV,MSG,homing from the manual zero (lift only). Send H1 for full auto homing."));
      state = ST_HOME_POS;
    } else {
      iqcal_pending = false;
      state = ST_HOME_NEG;               // phase 1: down to the lower stop first
    }
  }
}

// [DV1c, v2.9z] 'Z' — manual zero: the arm's CURRENT position becomes joint θ = 0.
// Only in IDLE/FAULT (arm limp), so it cannot jump a running controller. Put the arm on
// the lower stop by hand first. Before homing: 'H' then lifts only. After homing: the
// limits shift with the new zero and the device stays homed (re-zero for rope creep).
void doZero (char* cmd){ (void)cmd;
  if(state==ST_IDLE || state==ST_FAULT){
    float theta_now = s_theta;           // where the arm is, in the OLD zero frame
    home_motor_offset = motor.shaft_angle;
    zero_manual = true; zero_set = true;
    s_theta_ref = 0; s_vel_ref = 0;
    if (homed){
      lim_max = lim_max - theta_now;
      lim_min = HOME_MARGIN;
      if (lim_max - lim_min < MIN_RANGE){ homed = false; }   // nonsense shift → home again
      Serial.print(F("EV,ZERO,")); Serial.print(home_motor_offset, 4);
      Serial.print(','); Serial.println(homed ? 1 : 0);
      if (homed){
        Serial.print(F("EV,HOMED,")); Serial.print(lim_min, 4);
        Serial.print(',');            Serial.println(lim_max, 4);
        Serial.print(F("EV,MSG,zero moved here by ")); Serial.print(theta_now*57.2958f, 1);
        Serial.println(F(" deg - limits shifted, still homed"));
      } else {
        Serial.println(F("EV,MSG,zero set here, but the shift was too large - press Home (H) again"));
      }
    } else {
      Serial.print(F("EV,ZERO,")); Serial.print(home_motor_offset, 4); Serial.println(F(",0"));
      Serial.println(F("EV,MSG,zero set here. Press Home (H) to find the upper limit."));
    }
  } else {
    Serial.println(F("EV,MSG,Z refused - stop the motor first (O0), then put the arm at the bottom"));
  }
}

void doEst  (char* cmd){ int v = atoi(cmd); if(v>=0 && v<=2) est_sel = v; }

void doMode (char* cmd){
  int v = atoi(cmd);
  if (state == ST_LAB || motor.controller == MotionControlType::velocity){   // [v2.8] leave lab velocity mode ([DV1] also after a trip)
    lab_vel = 0.0f;
    motor.controller = MotionControlType::torque;
    motor.LPF_velocity.Tf = 0.005f;        // [v2.9] restore the estimator's velocity filter
  }
  if (v == 1 && tempBlocked()){ state = ST_IDLE; return; }   // [v2.9t]
  if(v==1 && homed){
    // bumpless entry into RUN: reference at current angle, estimators reset
    s_theta_ref = s_theta; s_vel_ref = 0;
    dob_integral = P_J*s_vel; dob_r = 0;   // r = ωc(Jθ̇ − integ) = 0 at entry
    vel_prev = s_vel; acc_filt = 0;
    state = ST_RUN;
  }
  else state = ST_IDLE;                    // O0 stops RUN and PASSIVE alike
}

void doAssist(char* cmd){ P_assist = atof(cmd); }

// [v2.8] lab velocity: V<rad/s motor shaft>. Enters ST_LAB from IDLE/FAULT;
// updates the target live while in ST_LAB. O0 exits back to torque mode.
void doLabVel(char* cmd){
  if (tempBlocked()) return;                // [v2.9t]
  lab_vel = clampf(atof(cmd), -300.0f, 300.0f);
  if (state == ST_IDLE || state == ST_FAULT || state == ST_LAB){
    if (state != ST_LAB){
      // [v2.9] velocity PID from the lab GUI g6-C: the SimpleFOC defaults
      // (P 0.5, I 10, Tf 5 ms) made the compliant capstan vibrate in E10.
      motor.PID_velocity.P = 0.25f; motor.PID_velocity.I = 3.0f; motor.PID_velocity.D = 0.0f;
      motor.PID_velocity.output_ramp = 1000.0f; motor.LPF_velocity.Tf = 0.04f;
      motor.PID_velocity.reset();
      motor.controller = MotionControlType::velocity;
      state = ST_LAB;
    }
  }
}

// [v2-2] passive goto — the GUI sequences sweeps with repeated G commands
void doGoto(char* cmd){
  if (tempBlocked()) return;                // [v2.9t]
  if(!homed) return;
  if (state == ST_IDLE) leaveLab();         // [DV1]
  passive_target = clampf(atof(cmd), lim_min, lim_max);
  if (state == ST_IDLE){                   // bumpless entry
    s_theta_ref = s_theta; s_vel_ref = 0;
    pass_integ = 0.0f;                     // [v2.5] fresh integral
    state = ST_PASSIVE;
  }
}

volatile uint32_t telem_period_ms = 10;   // default 100 Hz; D4 → 250 Hz; D2 → 500 Hz (§4.8 logging; ~48 kB/s at 921600 baud)
void doTelem(char* cmd){ telem_period_ms = (uint32_t)atoi(cmd); }

// [v2.9t] 'T' — thermistor dump: raw node volts first, because when a reading
// looks wrong the first question is always "what is the ADC actually seeing".
void doTemp(char* cmd){ (void)cmd;
  float ratio = therm_ratio();                       // [DV1] J9 on ADC2
  float v = therm_volts(ratio);
  float r = dv1_ntc_r(ratio, P_th_rfix);
  Serial.print(F("EV,TEMPDUMP,v="));  Serial.print(v, 4);
  Serial.print(F(",r_ntc="));         Serial.print(r, 1);
  Serial.print(F(",tempC="));         Serial.print(temp_c, 2);
  Serial.print(F(",ok="));            Serial.print((int)temp_ok);
  Serial.print(F(",src="));           Serial.print(temp_ok ? F("ntc") : F("model"));
  Serial.print(F(",T_est="));         Serial.print(T_est, 1);
  Serial.print(F(",warn="));          Serial.print(P_temp_warn, 1);
  Serial.print(F(",stop="));          Serial.print(P_temp_stop, 1);
  Serial.print(F(",tripped="));       Serial.print((int)temp_tripped);
  Serial.print(F(",mode="));          Serial.print(therm_mode);
  Serial.print(F(",beta="));          Serial.print(P_th_beta, 0);
  Serial.print(F(",r25="));           Serial.print(P_th_r25, 0);
  Serial.print(F(",rfix="));          Serial.print(P_th_rfix, 0);
  Serial.print(F(",T_fet="));         Serial.print((float)tfet_c, 1);   // [DV1]
  Serial.println(F(",ntc=J9-pullup"));
}

// [v2.9t] 'X<degC>' — solve beta from one reference temperature (as LabTest
// v2.7.1-R): beta = ln(R_ntc/R25) / (1/T_ref − 1/298.15). Assumes R25 is right;
// refuses references within 10 °C of 25 °C (denominator → 0). 60–80 °C bath.
void doCalBeta(char* cmd){
  float tref = atof(cmd);
  if (tref < -20.0f || tref > 150.0f){
    Serial.println(F("EV,MSG,X<degC>: give the bead's TRUE temperature, -20..150")); return; }
  if (fabsf(tref - 25.0f) < 10.0f){
    Serial.println(F("EV,MSG,X: too close to 25C to solve beta - use a bath 10C+ away (60-80C is ideal)")); return; }
  float ratio = dv1_adc2_ratio_avg(DV1_TMOT_CH, 16); // [DV1] J9 on ADC2
  float v = therm_volts(ratio);
  if (!dv1_ntc_ratio_ok(ratio)){
    Serial.print(F("EV,TEMPFAIL,")); Serial.println(v, 3);
    Serial.println(F("EV,MSG,X: cannot calibrate through a wiring fault")); return; }
  float r = dv1_ntc_r(ratio, P_th_rfix);
  float dinv = 1.0f / (tref + 273.15f) - 1.0f / 298.15f;
  float beta = logf(r / P_th_r25) / dinv;
  if (!isfinite(beta) || beta < 500.0f || beta > 8000.0f){
    Serial.print(F("EV,MSG,X: solved beta=")); Serial.print(beta, 0);
    Serial.println(F(" is not credible - check PN (R25) and the divider")); return; }
  P_th_beta = beta; temp_seeded = false;
  Serial.print(F("EV,CALBETA,")); Serial.print(beta, 0);
  Serial.print(F(",r_ntc="));     Serial.print(r, 0);
  Serial.print(F(",tref="));      Serial.println(tref, 1);
  Serial.println(F("EV,MSG,runtime only - put this in P_th_beta in main.cpp to keep it"));
}

// status dump — check the sensor is alive (rotate by hand, raw angle must change)
void doStat(char* cmd){ (void)cmd;
  Serial.print(F("L state="));  Serial.print((int)state);
  Serial.print(F(" homed="));   Serial.print(homed);
  Serial.print(F(" raw_motor_angle=")); Serial.print(motor.shaft_angle, 3);
  Serial.print(F(" joint_deg=")); Serial.print(s_theta*57.2958f, 1);
  Serial.print(F(" iq_sign=")); Serial.print(P_iq_sign, 0);
  Serial.print(F(" limits_deg=[")); Serial.print(lim_min*57.2958f, 1);
  Serial.print(F(",")); Serial.print(lim_max*57.2958f, 1); Serial.print(F("]"));
  Serial.print(F(" Ktj=")); Serial.print(P_Ktj, 4);            // [v2.9]
  Serial.print(F(" N=")); Serial.print(N_CAPSTAN, 2);
  Serial.print(F(" R=")); Serial.print(motor.phase_resistance, 3);
  Serial.print(F(" L=")); Serial.print(motor.phase_inductance * 1000.0f, 3); Serial.print(F("mH"));
  Serial.print(F(" dz=")); Serial.print(P_dz, 3);
  Serial.print(F(" T_est=")); Serial.print(T_est, 1);
  Serial.print(F(" T_src=")); Serial.print(temp_ok ? ((therm_mode == 1 && state != ST_IDLE && state != ST_FAULT) ? F("ntc+model") : F("ntc")) : F("model"));   // [v2.9t]
  Serial.print(F(" T_mode=")); Serial.print(therm_mode);
  Serial.print(F(" T_warn=")); Serial.print(P_temp_warn, 0);
  Serial.print(F(" T_stop=")); Serial.print(P_temp_stop, 0);
  Serial.print(F(" T_trip=")); Serial.print((int)temp_tripped);
  Serial.print(F(" iq_lim=")); Serial.print(fminf(P_iq_lim, iq_lim_thermal), 2);
  Serial.print(F(" fw=")); Serial.print(F(FW_TAG));      // GUIs check fw=v2.9t: keep this token
  // [DV1] board fields, AFTER fw= so every existing key and the fw token are unchanged
  Serial.print(F(" hw=DV1 build=")); Serial.print(F(FW_FULL));
  Serial.print(F(" foc="));      Serial.print(foc_ready ? F("OK") : F("FAIL"));
  Serial.print(F(" drv="));      Serial.print(drv_fault ? F("FAULT") : (drv_cfg_ok ? F("OK") : F("NOCFG")));
  Serial.print(F(" nfault="));   Serial.print((int)dv1_nfault_low());
  Serial.print(F(" vbus="));     Serial.print(vbus_now, 2);
  Serial.print(F(" T_fet="));    Serial.print((float)tfet_c, 1);
  Serial.print(F(" fet_trip=")); Serial.print((int)fet_tripped);
  Serial.print(F(" cs_align=")); Serial.print(cs_align);
  Serial.print(F(" pwm="));      Serial.print(EXARMO_PWM6 ? F("6x") : F("3x"));
  Serial.print(F(" clk="));      Serial.print(dv1_clock_source());
  // [DV1c] homing / gravity / zero
  Serial.print(F(" Ph="));       Serial.print(P_iq_home, 2);
  Serial.print(F(" Ghome="));    Serial.print(P_Ghome, 3);
  Serial.print(F(" phi_home=")); Serial.print(P_Ghome_phase, 3);
  Serial.print(F(" g="));        Serial.print(P_Gcoef, 3);
  Serial.print(F(" phi="));      Serial.print(P_Gphase, 3);
  Serial.print(F(" Rmin_deg=")); Serial.print(P_range_min*57.2958f, 1);
  Serial.print(F(" zero="));     Serial.println(zero_manual ? F("manual") : (zero_set ? F("auto") : F("none")));
}

// [DV1c] 'S' — SOFT STOP (v2.9z t6, the STOP buttons of ExArMoRehab_v2.9.3 / ExArMoTouch
// and the end of a Passive set): from RUN / PASSIVE the arm is lowered slowly to 0°
// (lim_min) by the park controller at 'Py' rad/s with the session gravity model, and
// released only once it rests there — instead of dropping it. 'S' (or O0) again while it
// is lowering = release immediately. Any other state → plain stop (O0).
// The timeout grows with the distance to go (distance / Py + 4 s), so a slow lowering
// from the top is never cut short into a drop.
void doStopS(char* cmd){ (void)cmd;
  if ((state == ST_RUN || state == ST_PASSIVE) && homed && s_theta > lim_min + 0.05f){
    s_theta_ref = s_theta; s_vel_ref = 0.0f;   // start the ramp from where the arm is
    float spd = fmaxf(P_park_speed, 0.05f);
    park_timeout = (uint32_t)(1000.0f * (s_theta - lim_min) / spd) + 4000u;
    park_goal = lim_min;
    state = ST_HOME_CENTER;
    Serial.println(F("EV,SOFTSTOP,1"));
    Serial.println(F("EV,MSG,soft stop - lowering to 0, then release (S or O0 again = release now)"));
  } else if (state == ST_HOME_CENTER && park_goal >= 0.0f){
    park_goal = -1.0f; park_timeout = PARK_TIMEOUT;
    state = ST_IDLE;                           // second press: release now
    Serial.println(F("EV,SOFTSTOP,0"));
  } else {
    doMode((char*)"0");                        // homing / lab / already low / idle → plain stop
  }
}

// [DV1] 'Q<n>' encoder check (IDLE/FAULT only): n raw AS5047P reads, 100 ms apart, then
// a verdict. Each read pauses the 20 kHz FOC timer for ~60 us (SimpleFOC reads the same
// sensor on SPI1 in that ISR). Flex the arm by hand during 'Q20' to see the angle move.
void doEncQ(char* cmd){
  int n = atoi(cmd); if (n < 1) n = 1; if (n > 50) n = 50;
  if (state != ST_IDLE && state != ST_FAULT){ Serial.println(F("EV,MSG,Q: send O0 first (the check pauses the FOC loop)")); return; }
  int ffff = 0, zero = 0, par = 0, mag = 0; uint16_t amin = 0xFFFF, amax = 0;
  for (int i = 0; i < n; i++){
    DV1EncSample e;
    if (ctrl_timer) ctrl_timer->pause();
    dv1_as5047_sample(&SPI_1, e);
    if (ctrl_timer) ctrl_timer->resume();
    dv1_as5047_print(Serial, e, i);
    if (e.dia == 0xFFFF && e.com == 0xFFFF) ffff++;
    if (e.dia == 0x0000 && e.com == 0x0000) zero++;
    par += e.parity_bad;
    if ((e.dia >> 10) & 3) mag++;
    uint16_t a = e.com & 0x3FFF; if (a < amin) amin = a; if (a > amax) amax = a;
    if (i + 1 < n) delay(100);
  }
  dv1_as5047_verdict(Serial, n, ffff, zero, par, mag, amin, amax);
  Serial.print(F("EV,MSG,SimpleFOC view: mech_deg="));  Serial.print(sensor.getMechanicalAngle() * 57.2958f, 2);
  Serial.print(F(" sensor_direction="));              Serial.print((int)motor.sensor_direction);
  Serial.print(F(" zea="));                           Serial.print(motor.zero_electric_angle, 4);
  Serial.print(F(" foc="));                           Serial.println(foc_ready ? F("OK") : F("FAIL"));
}

// [DV1] 'F' gate-driver report; 'F1' (IDLE/FAULT only) re-writes the DRV8320S
// settings and clears a latched fault.
void doDrv(char* cmd){
  if (atoi(cmd) == 1){
    if (state != ST_IDLE && state != ST_FAULT){ Serial.println(F("EV,MSG,F1: send O0 first")); return; }
    drv_cfg_ok = dv1_drv_configure();
    delayMicroseconds(50);
    drv_fault = dv1_nfault_low();
    Serial.print(F("EV,DRV,")); Serial.print((int)drv_cfg_ok);
    Serial.println(drv_fault ? F(",nFAULT still low") : F(",cleared"));
  }
  dv1_drv_print_regs(Serial);
  Serial.print(F("EV,MSG,vbus=")); Serial.print(vbus_now, 2);
  Serial.print(F(" T_fet="));      Serial.print((float)tfet_c, 1);
  Serial.print(F(" foc="));        Serial.print(foc_ready ? F("OK") : F("FAIL"));
  Serial.print(F(" cs_align="));   Serial.print(cs_align);
  Serial.print(F(" pwm="));        Serial.print(EXARMO_PWM6 ? F("6x") : F("3x"));
  Serial.print(F(" clk="));        Serial.println(dv1_clock_source());
}

void doParam(char* cmd){
  char k = cmd[0]; float v = atof(&cmd[1]);
  switch(k){
    case 'g': P_Gcoef=v; break;  case 'f': P_Gphase=v; break;
    case 'j': P_J=v;     break;  case 'b': P_B=v;      break;
    case 'J': P_Jv=v;    break;  case 'n': P_Bmin=v;   break;
    case 'x': P_Bmax=v;  break;  case 't': P_tau0=v;   break;
    case 'w': P_wc=v;    break;  case 'p': P_Kp=v;     break;
    case 'd': P_Kd=v;    break;  case 'i': P_iq_lim=v; motor.current_limit=v; break;
    case 'h': P_iq_home=clampf(v, 0.3f, 3.0f); break;
    case 'G': P_Ghome = clampf(v, 0.0f, 2.0f); break;          // [DV1c] homing/park gravity [Nm]
    case 'F': P_Ghome_phase = clampf(v, -3.2f, 3.2f); break;   // [DV1d] homing/park gravity phase [rad]
    case 'R': P_range_min = clampf(v, MIN_RANGE, 3.2f); break; // [DV1c] expected range [rad]
    case 'v': P_pass_speed=v; break;   // [v2-2]
    case 'a': P_pass_acc = fmaxf(0.05f, v); break;   // [v2.4] profile accel
    case 'c': P_fric = clampf(v, 0.0f, 0.5f);  break;   // [v2.4] friction FF
    case 'e': P_Ki_pass = clampf(v, 0.0f, 40.0f); break; // [v2.5] passive integral
    case 'm': {                                          // [v2.5] torque mode:
      int tm = (int)v;                                   // 0 voltage (smoothest if
      motor.torque_controller =                          //   current sense is off),
          (tm==0) ? TorqueControlType::voltage           // 1 dc_current, 2 foc_current
        : (tm==1) ? TorqueControlType::dc_current
                  : TorqueControlType::foc_current;
      Serial.print(F("torque_mode=")); Serial.println(tm);
      break; }
    case 'k': P_park=v;       break;   // [v2-3]
    case 'y': P_park_speed=v; break;   // [v2-3]
    case 'q': P_iq_sign = (v < 0) ? -1.0f : 1.0f; break;   // [v2.3] manual override
    case 'K': P_Ktj = clampf(v, 0.2f, 2.0f); break;        // [v2.9] joint torque gain
    case 'z': P_dz  = clampf(v, 0.0f, 1.0f); break;        // [v2.9] dead-zone
    case 'u': P_icont = clampf(v, 0.5f, 8.0f); break;      // [v2.9] continuous Iq
    case 'r': P_R = clampf(v, 0.05f, 5.0f); motor.phase_resistance = P_R;   // [v2.9]
              Serial.print(F("tune=")); Serial.println(motor.tuneCurrentController(100.0f)); break;
    case 'l': P_L = clampf(v, 1e-6f, 0.01f); motor.phase_inductance = P_L;  // [v2.9]
              motor.axis_inductance = {P_L, P_L};
              Serial.print(F("tune=")); Serial.println(motor.tuneCurrentController(100.0f)); break;
    case 'N': P_th_r25  = clampf(v, 100.0f, 1.0e6f); temp_seeded = false; break;   // [v2.9t]
    case 'O': P_th_rfix = clampf(v, 100.0f, 1.0e6f); temp_seeded = false; break;
    case 'B': P_th_beta = clampf(v, 500.0f, 8000.0f); temp_seeded = false; break;
    case 'T': P_th_off  = clampf(v, -20.0f, 20.0f);   temp_seeded = false; break;
    case 'W': P_temp_warn = clampf(v, 20.0f, 120.0f); break;
    case 'S': P_temp_stop = (v <= 0.0f) ? 0.0f : clampf(v, 30.0f, 120.0f); break;
    case 'A': therm_mode = (int)clampf(v, 0.0f, 2.0f); if (therm_mode == 0){ temp_ok = false; temp_seeded = false; } break;   // bead mode 0/1/2
    default: Serial.println(F("P? g f j b J n x t w p d i h v k y q a c e m K z u r l N O B T W S A  G F R  (also T, X<degC>, F, S, Q, Z, H1)")); return;
  }
  Serial.print(F("P ")); Serial.print(k); Serial.print('='); Serial.println(v, 4);
}

// ======================= setup ====================================
void setup(){
  dv1_led_begin();                      // [DV1] LEDs off, brake switch off
  digitalWrite(DV1_LED_FLT, HIGH);      // red while booting
  _delay(6000);
  Serial.begin(921600);                 // high baud for telemetry; match in the Python app
  SimpleFOCDebug::enable(&Serial);

  // [DV1] VBUS (PC3), FET NTC (PC4), motor NTC (PC5) = analogue inputs on ADC2,
  // set up before driver/current-sense touch any ADC.
  dv1_adc2_init();
  float v = dv1_read_vbus();
  // [DV1] USB alone powers the MCU (D3) but not the DRV8320S. Give the operator
  // 15 s to switch the 24 V on before carrying on regardless.
  if (!(v >= DV1_VBUS_MIN)){
    Serial.print(F("EV,VBUS,")); Serial.println(v, 2);
    Serial.println(F("EV,MSG,no motor supply on the XT30 - waiting up to 15 s (USB powers the MCU only)"));
    uint32_t t0 = millis();
    while (!(v >= DV1_VBUS_MIN) && millis() - t0 < 15000){
      delay(250); v = dv1_read_vbus();
      digitalWrite(DV1_LED_FLT, ((millis() / 250) & 1) ? HIGH : LOW);
    }
    digitalWrite(DV1_LED_FLT, HIGH);
  }
  // [v2.9] lab fix v2-F: a bad VSENS read (0 V) silently caps every phase
  // voltage at zero — no torque, no current, no error. Guard and announce.
  // [DV1] board window 8..32 V: below = assume 24 V (as before); above = keep the
  // MEASURED value (PWM scaling must use the real bus) and warn.
  if (!(v > DV1_VBUS_MIN && v < DV1_VBUS_MAX)){
    Serial.print(F("EV,VBUS,")); Serial.println(v, 2);
    if (v >= DV1_VBUS_MAX) Serial.println(F("EV,MSG,VBUS ABOVE the 30 V board rating - lower the supply!"));
    else { Serial.println(F("EV,MSG,VSENS read out of range - assuming 24 V")); v = 24.0f; }
  }
  vbus_meas = v; vbus_now = v;
  SIMPLEFOC_DEBUG(" V sens: ", v);

  // [DV1] DRV8320S: wake (ENABLE high, for good) and configure over SPI3 BEFORE
  // the PWM starts. Replaces the DRV8301 gain write of the MKS board.
  drv_cfg_ok = dv1_drv_begin(&SPI_3);
  {
    char txt[96]; dv1_drv_fault_text(dv1drv.fault1, dv1drv.fault2, txt, sizeof(txt));
    Serial.print(F("EV,DRV,")); Serial.print((int)drv_cfg_ok);
    Serial.print(F(",ctrl=0x")); Serial.print(dv1drv.reg[2], HEX);
    Serial.print(F(",hs=0x"));   Serial.print(dv1drv.reg[3], HEX);
    Serial.print(F(",ls=0x"));   Serial.print(dv1drv.reg[4], HEX);
    Serial.print(F(",ocp=0x"));  Serial.print(dv1drv.reg[5], HEX);
    Serial.print(F(",faults=")); Serial.println(txt);
    if (!drv_cfg_ok)
      Serial.println(F("EV,MSG,DRV8320S did not take its settings - no 24 V, or SPI3 (PC10/11/12, nSCS PA15) - motion is blocked"));
  }

  driver.voltage_power_supply = v;
  driver.voltage_limit = v;
#if EXARMO_PWM6
  driver.dead_zone = 0.0025f;           // [DV1] 100 ns at 25 kHz; the DRV8320S adds its own
#endif
  driver.init();
  delay(10);

  motor.linkDriver(&driver);
  sensor.init(&SPI_1);                  // [DV1] encoder on SPI1 / J11
  dv1_as5047_diag(&SPI_1, Serial);      // [DV1] magnet + cable check, once
  motor.linkSensor(&sensor);

  motor.torque_controller = TorqueControlType::foc_current;
  motor.controller = MotionControlType::torque;
  motor.voltage_limit = 24;
  motor.voltage_sensor_align = 1.5;
  motor.current_limit = P_iq_lim;
  motor.velocity_limit = 350;   // [v2.8] rad/s motor — lab mode needs headroom
                                // (SimpleFOC default 20 rad/s = only ~190 rpm)
  // [v2.9] electrical parameters BEFORE initFOC, so tuneCurrentController()
  // is deterministic and never runs characteriseMotor() at boot (lab fix v2-E)
  motor.phase_resistance = P_R;
  motor.phase_inductance = P_L;
  motor.axis_inductance  = {P_L, P_L};
  motor.KV_rating = KV_MEAS;    // E02: 123 ± 1 rpm/V (was 114.1, single motor, July)

  motor.useMonitoring(Serial);
  motor.monitor_variables = _MON_CURR_Q | _MON_ANGLE;
  motor.monitor_separator = ',';
  motor.monitor_end_char = 'M';
  motor.monitor_start_char = 'M';
  motor.monitor_downsample = 0;

  // commander
  command.add('M', doMotor,  "motor M0");
  command.add('H', doHome,   "home capstan limits (H1 = full auto)");
  command.add('Z', doZero,   "manual zero: current position = joint 0");   // [DV1c, v2.9z]
  command.add('E', doEst,    "estimator 0/1/2");
  command.add('O', doMode,   "0 idle 1 run");
  command.add('A', doAssist, "assist gain");
  command.add('G', doGoto,   "passive goto [rad joint]");   // [v2-2]
  command.add('V', doLabVel, "lab velocity [rad/s motor]"); // [v2.8]
  command.add('D', doTelem,  "telemetry period ms");
  command.add('L', doStat,   "status dump");
  command.add('P', doParam,  "params");
  command.add('T', doTemp,   "thermistor dump");                 // [v2.9t]
  command.add('X', doCalBeta,"solve beta from a reference temp"); // [v2.9t]
  command.add('S', doStopS,  "soft stop: lower to 0 then release (S again = release now)");   // [DV1c]
  command.add('F', doDrv,    "gate-driver report, F1 = clear");   // [DV1]
  command.add('Q', doEncQ,   "encoder check, Q<n> = n reads");    // [DV1]

  commandc.init();
  commandc.addMotor(&motor);

  motor.init();
  current_sense.linkDriver(&driver);
  bool cs_ok = current_sense.init();
  // [DV1] injected sample time 3 -> 15 ADC cycles on PC0/PC1/PC2 (ch10..12): each
  // INA240 drives the pin through 100R with no capacitor fitted; 3 phases still
  // convert in ~3.9 us of the 40 us PWM period.
  if (cs_ok) ADC1->SMPR1 = (ADC1->SMPR1 & ~0x1FFu) | (1u << 0) | (1u << 3) | (1u << 6);
  else Serial.println(F("EV,MSG,current_sense.init() FAILED - Iq will read 0"));
  current_sense.skip_align = true;
#if EXARMO_CS_ALIGN
  // [DV1] current-sense phase/polarity check: phase A driven alone at
  // voltage_sensor_align must read as the largest, positive current. Works with the
  // arm held. 1 ok · 2 order fixed · 3 polarity fixed · 4 both · 0 FAILED.
  if (cs_ok && drv_cfg_ok){
    current_sense.skip_align = false;
    cs_align = current_sense.driverAlign(motor.voltage_sensor_align, motor.modulation_centered);
    current_sense.skip_align = true;
  } else cs_align = 0;
  Serial.print(F("EV,CSALIGN,")); Serial.print(cs_align);
  Serial.print(','); Serial.print(current_sense.gain_a, 3);
  Serial.print(','); Serial.print(current_sense.gain_b, 3);
  Serial.print(','); Serial.println(current_sense.gain_c, 3);
  if (cs_align == 0) Serial.println(F("EV,MSG,current-sense check FAILED - motion is blocked. 24 V on? Motor connected? Press RESET to retry."));
  else if (cs_align > 1) Serial.println(F("EV,MSG,current-sense mapping differed from the schematic and was corrected - note the gains above"));
#endif
  motor.linkCurrentSense(&current_sense);
  // [DV1] the MKS build ignored this result; a failed init now blocks H/O1/G/V
  foc_ready = (motor.initFOC() == 1) && cs_ok && (!EXARMO_CS_ALIGN || cs_align >= 1);
  if (!foc_ready) Serial.println(F("EV,FOCFAIL,initFOC failed - check the encoder (EV,ENC), the motor phases and the 24 V. Motion refused."));

  Serial.print(F("EV,TUNE,")); Serial.println(motor.tuneCurrentController(100.0f));  // [v2.9] R, L already set
  delay(200);

  motor.motion_downsample = 0;          // we call move() ourselves at 1 kHz

  HardwareTimer* timer = new HardwareTimer(TIM8);
  ctrl_timer = timer;                   // [DV1] for 'Q'
  timer->setOverflow(20000, HERTZ_FORMAT);
  static int ctr = 0;
  timer->attachInterrupt([](){
    motor.loopFOC();
    if(++ctr >= CTRL_DIV){ ctr = 0; controlUpdate(); }
  });
  timer->resume();

  // [v2.9t] first thermistor read AFTER the control timer is live, so an ADC
  // conflict with the current sense would show up here at boot
  temp_last_ms = millis();
  thermUpdate();
  if (temp_ok){ Serial.print(F("EV,TEMPOK,")); Serial.println(temp_c, 1); }
  else          Serial.println(F("EV,MSG,no thermistor on J9 MOTOR NTC (PC5) - using the software thermal model"));
  ev_tempok = false; ev_tempfail = false;   // announced right here, once
  fetUpdate();                              // [DV1]
  Serial.print(F("EV,MSG,Driver V1: clock=")); Serial.print(dv1_clock_source());
  Serial.print(F(" pwm="));   Serial.print(EXARMO_PWM6 ? F("6x") : F("3x"));
  Serial.print(F(" T_fet=")); Serial.print((float)tfet_c, 1);
  Serial.print(F(" drv="));   Serial.print(drv_cfg_ok ? F("OK") : F("NOCFG"));
  Serial.print(F(" foc="));   Serial.println(foc_ready ? F("OK") : F("FAIL"));
  digitalWrite(DV1_LED_FLT, LOW);           // boot done; dv1_led_update() takes over

  Serial.print(F("READY v2.9t-DV1 (Driver V1; Ktj ")); Serial.print(P_Ktj, 3);
  Serial.println(F(" (DV1 interim), N 8.68, R 0.47, dz 0.15, thermistor J9: warn 40 C, stop 55 C)."));
  Serial.println(F("EV,MSG,Ktj 0.690 is the Driver V1 interim value (0.760 MKS-amps / 1.10): measure it (E09 0 deg) and set PK. Ph default 1.3 A (= 1.2 MKS-A)."));
  Serial.println(F("Send H to home, then O1 to run. S = soft stop (lower to 0, release). D0 stops telemetry. T = thermistor dump."));
}

// ======================= loop: telemetry & comms ==================
void loop(){
  // ---- events queued from the ISR (Serial inside the ISR is unsafe) ----
  if (evt){
    uint8_t e = evt; float ea = evt_a, eb = evt_b; evt = 0;
    switch(e){
      case 1: Serial.print(F("EV,STOP_LOW,iq_sign=")); Serial.println(ea, 0);  break;
      case 2: Serial.print(F("EV,HOMED,"));  Serial.print(ea, 4);
              Serial.print(',');             Serial.println(eb, 4);           break;
      case 3: Serial.print(F("EV,HOMEFAIL,")); Serial.println(ea, 4);
              Serial.println(F("EV,MSG,range too small - raise Ph or check mechanism")); break;
      case 4: Serial.print(F("EV,PARKED,")); Serial.println(ea, 4);           break;
      case 5: Serial.print(F("EV,RANGE_FAULT,")); Serial.println(ea, 4);      break;
      case 6: Serial.println(F("EV,HOMETIMEOUT,0"));                          break;
      case 7: Serial.print(F("EV,THERM,")); Serial.print(ea, 1);             // [v2.9]
              Serial.print(','); Serial.println(eb, 2);                       break;
    }
  }

  // ---- [DV1c] homing / soft-stop events (after EV,HOMED of the same tick) ----
  if (ev_homeboost){ ev_homeboost = false;
    Serial.print(F("EV,HOMEBOOST,")); Serial.print(ev_hb_a, 4);
    Serial.print(','); Serial.println(ev_hb_b, 2); }
  if (ev_homeshort){ ev_homeshort = false;
    Serial.print(F("EV,HOMESHORT,")); Serial.println(ev_hs, 4);
    Serial.print(F("EV,MSG,homed, but the range is only ")); Serial.print(ev_hs*57.2958f, 0);
    Serial.println(F(" deg (< PR) - rope friction? If this IS the real range, set PR a few deg lower.")); }
  if (ev_softstop_done){ ev_softstop_done = false;
    Serial.println(F("EV,MSG,soft stop done - arm at 0, motor released")); }

  // ---- [v2.9t] thermistor at 5 Hz + its events ----
  if (millis() - temp_last_ms >= 200){
    temp_last_ms = millis(); thermUpdate();
    fetUpdate();                                     // [DV1] FET-board NTC
    float vb = dv1_read_vbus();                      // [DV1] live bus voltage (ADC2)
    if (vb >= 0.0f) vbus_now = vb;
  }
  // ---- [DV1] gate-driver / FET events and a 1 Hz settings check ----
  if (ev_drvfault){ ev_drvfault = false;
    dv1_drv_read_all();
    char txt[96]; dv1_drv_fault_text(dv1drv.fault1, dv1drv.fault2, txt, sizeof(txt));
    Serial.print(F("EV,DRVFAULT,0x")); Serial.print(dv1drv.fault1, HEX);
    Serial.print(F(",0x")); Serial.print(dv1drv.fault2, HEX);
    Serial.print(','); Serial.println(txt);
    Serial.println(F("EV,MSG,gate driver tripped - FAULT (bridge off, arm free). The next H/O1/G/V clears it; 'F' shows the registers.")); }
  if (ev_fettrip){ ev_fettrip = false;
    Serial.print(F("EV,FETTEMP,")); Serial.println((float)tfet_c, 1);
    Serial.println(F("EV,MSG,POWER STAGE TOO HOT (FET NTC) - stopped (coast, arm free). Let it cool.")); }
  {
    static uint32_t last_drv = 0;
    if (millis() - last_drv >= 1000){
      last_drv = millis();
      bool ok = dv1_drv_verify();
      if (!ok && drv_cfg_ok){
        // lost its settings (VM brown-out resets it to 6x PWM, where INLx high
        // would mean "low side on"): coast first, then re-write
        drv_cfg_ok = false;
        if (state != ST_IDLE && state != ST_FAULT){
          if (state == ST_LAB){ lab_vel = 0.0f; motor.controller = MotionControlType::torque; motor.LPF_velocity.Tf = 0.005f; }
          state = ST_FAULT;
        }
        Serial.print(F("EV,DRV,0,settings lost (VBUS=")); Serial.print(vbus_now, 1);
        Serial.println(F(" V) - FAULT, re-writing"));
      }
      if (!ok && vbus_now >= DV1_VBUS_MIN){
        drv_cfg_ok = dv1_drv_configure();
        if (drv_cfg_ok){
          Serial.println(F("EV,DRV,1,configured"));
          if (!foc_ready) Serial.println(F("EV,MSG,gate driver ready now, but the motor was not initialised at boot - press RESET"));
        }
      } else if (ok) {
        drv_cfg_ok = true;
      }
    }
  }
  dv1_led_update(state != ST_IDLE && state != ST_FAULT,
                 drv_fault || !drv_cfg_ok || !foc_ready || temp_tripped || fet_tripped || state == ST_FAULT,
                 derating || !(vbus_now > DV1_VBUS_MIN && vbus_now < DV1_VBUS_MAX) ||
                 (isfinite(tfet_c) && tfet_c > DV1_TFET_WARN));
  if (ev_temptrip){ ev_temptrip = false;
    Serial.print(F("EV,TEMP,")); Serial.println(ev_t, 1);
    Serial.println(F("EV,MSG,MOTOR TOO HOT - stopped (coast, arm free). Let it cool; O1/G/H/V refused until then.")); }
  if (ev_tempclear){ ev_tempclear = false;
    Serial.print(F("EV,TEMPCLEAR,")); Serial.println(ev_t, 1); }
  if (ev_tempwarn){ ev_tempwarn = false;
    Serial.print(F("EV,TEMPWARN,")); Serial.println(ev_t, 1); }
  if (ev_tempfail){ ev_tempfail = false;
    Serial.print(F("EV,TEMPFAIL,")); Serial.println(ev_t, 3);
    Serial.println(F("EV,MSG,thermistor open/shorted - falling back to the software model (T dumps the raw volts)")); }
  if (ev_tempok){ ev_tempok = false;
    Serial.print(F("EV,TEMPOK,")); Serial.println(ev_t, 1); }

  // ---- telemetry ----
  static uint32_t last = 0;
  uint32_t now = millis();
  if (telem_period_ms > 0 && now - last >= telem_period_ms){
    last = now;
    // D,t_ms,state,theta,theta_ref,vel,iq_meas,tau_m,tau_s,tau_d,tau_dob,Bv,iq_cmd,T_est
    Serial.print("D,"); Serial.print(now);            Serial.print(',');
    Serial.print((int)state);                          Serial.print(',');
    Serial.print(s_theta, 4);                          Serial.print(',');
    Serial.print(s_theta_ref, 4);                      Serial.print(',');
    Serial.print(s_vel, 4);                            Serial.print(',');
    Serial.print(P_iq_sign*motor.current.q, 3);        Serial.print(',');
    Serial.print(s_tau_m, 4);                          Serial.print(',');
    Serial.print(s_tau_s, 4);                          Serial.print(',');
    Serial.print(s_tau_d, 4);                          Serial.print(',');
    Serial.print(s_tau_dob, 4);                        Serial.print(',');
    Serial.print(s_Bv, 3);                             Serial.print(',');
    Serial.print(s_iq_cmd, 3);                         Serial.print(',');
    Serial.println(T_est, 1);                          // [v2.9] field 13
  }
  motor.monitor();
  command.run();
  commandc.run();
}
