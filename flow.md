## Flowchart (based on final code)

```mermaid
flowchart TD
  %% --- STARTUP ---
  A[Start / setup()] --> B[Init pins, servo center, stopAll()]
  B --> C[Main loop]

  %% --- OBSTACLE FIRST ---
  C --> D{anyObstacle()? (IR_L / IR_F / IR_R)}
  D -- Yes --> E[avoidByIR():\nstop → back/turn combos → stop]
  E --> C

  %% --- MODE SWITCH ---
  D -- No --> F{mode == ACQUIRE ?}
  F -- Yes --> G[acquire():\nsweep servo Right→Center→Left,\npick best distance/angle,\nturn toward best, update lastSeenMs]
  G --> H{acquire() success?}
  H -- Yes --> I[mode = FOLLOW;\nlastAcquireMs = now]
  I --> C
  H -- No --> J[turnRight(); delay(130);\nstopAll(); delay(30)]
  J --> C

  %% --- FOLLOW MODE ---
  F -- No --> K[distCM() → d;\nif valid update lastSeenMs]
  K --> L{d < backLimit? (too close)}
  L -- Yes --> M[stop → back(130ms) → stop]
  M --> C
  L -- No --> N{d >= farLimit\nOR now-lastSeenMs > lostTimeoutMs\nOR now-lastAcquireMs > reacquireEveryMs}
  N -- Yes --> O[mode = ACQUIRE]
  O --> C
  N -- No --> P{d > maxFollow?\n(d < minFollow?)}
  P -- d > maxFollow --> Q[forward(); delay(170);\nstopAll(); delay(10)]
  P -- d < minFollow and > backLimit --> R[back(); delay(100);\nstopAll(); delay(10)]
  P -- within [min..max] --> S["comfort band:\nshort forward(90ms) → stop(8ms)"]
  Q --> C
  R --> C
  S --> C
