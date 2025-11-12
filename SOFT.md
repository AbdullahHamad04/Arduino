```mermaid
flowchart LR
  Start([start]) --> Init([initialize pins, servo center, mode = ACQUIRE])
  Init --> Loop([main loop])

  subgraph MainLoop
    Loop --> CheckIR{any IR obstacle?}
    CheckIR -- yes --> Avoid([avoidByIR() -> stopAll/back/turn])
    Avoid --> Loop

    CheckIR -- no --> ModeCheck{mode == ACQUIRE?}
    ModeCheck -- yes --> Acquire([acquire(): servo scan R → C → L\nmeasure distances, decide best angle])
    Acquire --> Found{valid target within range?}
    Found -- yes --> SetFollow([set mode = FOLLOW\nlastSeen = now\nlastAcquire = now])
    SetFollow --> Loop
    Found -- no --> Rotate([turnRight short])
    Rotate --> Loop

    ModeCheck -- no --> Follow([follow(): read front distance d])
    Follow --> UpdateDist([update lastSeen if d valid])
    UpdateDist --> TooClose{d < backLimit?}
    TooClose -- yes --> Back([back + stop])
    Back --> Loop
    TooClose -- no --> Lost{d >= farLimit or timeout or reacquireDue?}
    Lost -- yes --> SetAcquire([mode = ACQUIRE])
    SetAcquire --> Loop
    Lost -- no --> InRange{minFollow <= d <= maxFollow?}
    InRange -- yes --> Gentle([gentle forward pulses])
    Gentle --> Loop
    InRange -- no --> TooFar{d > maxFollow?}
    TooFar -- yes --> ForwardBurst([forward pulse])
    TooFar -- no --> ShortBack([small back pulse])
    ForwardBurst --> Loop
    ShortBack --> Loop
  end

  classDef decision fill:#f9f,stroke:#333,stroke-width:1px;
  class CheckIR,ModeCheck,Found,TooClose,Lost,InRange,TooFar decision;
```
