```mermaid
flowchart TD

A([Power ON]) --> B[Arduino UNO boots\ninitialises sensors & L298N]

B --> C[IR sensors (LM393)\nread for obstacles]

C --> D{Obstacle detected ?}
D -->|Yes| E[Arduino triggers avoid routine:\nstop motors → back → turn]
E --> C

D -->|No| F[Ultrasonic HC-SR04\nmeasure distance to human]

F --> G{Human too far ?}
G -->|Yes| H[Arduino sends PWM to L298N\nto drive 4 DC motors forward]
H --> C

G -->|No| I{Human too close ?}
I -->|Yes| J[Arduino commands motors\nshort reverse]
J --> C

I -->|No| K[Hold comfort distance:\nsmall forward pulses]
K --> C
```
