```mermaid
flowchart TD

A([Start]) --> B[Initialize Arduino, sensors and motors]

B --> C{Obstacle detected by IR ?}
C -->|Yes| D[Run avoidByIR routine<br>stop / back / turn]
D --> B

C -->|No| E{Mode == ACQUIRE ?}

E -->|Yes| F[Run acquire()<br>servo sweep right-center-left<br>pick best distance]
F --> G{acquire success ?}
G -->|Yes| H[Set mode = FOLLOW]
H --> B
G -->|No| I[Small right turn<br>then stop]
I --> B

E -->|No| J[Read distance d with ultrasonic]

J --> K{d < backLimit ?}
K -->|Yes| L[Back a little then stop]
L --> B

K -->|No| M{d >= farLimit<br>OR lost timeout<br>OR reacquire timeout ?}
M -->|Yes| N[Set mode = ACQUIRE]
N --> B

M -->|No| O{d > maxFollow ?}
O -->|Yes| P[Forward short pulse]
P --> B

O -->|No| Q{d < minFollow ?}
Q -->|Yes| R[Back short pulse]
R --> B

Q -->|No| S[Short forward pulse inside comfort zone]
S --> B
```
