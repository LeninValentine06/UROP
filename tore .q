[33mcommit 419d533d1b46c8e32c4c72ebc4906eb6b8660b47[m[33m ([m[1;36mHEAD[m[33m -> [m[1;32mdisplay-lvgl[m[33m, [m[1;31morigin/display-lvgl[m[33m)[m
Author: LeninValentine06 <leninvalentine97@gmail.com>
Date:   Sat Jan 31 11:50:39 2026 +0530

    Started display ui using lvgl

[33mcommit ffce1296b903a199a16c83cd00c70f3b35476c57[m[33m ([m[1;31morigin/main[m[33m, [m[1;31morigin/HEAD[m[33m, [m[1;32mmain[m[33m)[m
Author: LeninValentine06 <leninvalentine97@gmail.com>
Date:   Mon Jan 26 01:28:48 2026 +0530

    Updated files

[33mcommit 8a52e6ff6457dcd0ed32db46957c862d5e0f4c9f[m
Author: Lenin Valentine C J <93607279+LeninValentine06@users.noreply.github.com>
Date:   Fri Jan 23 17:31:40 2026 +0530

    Add README for resource-efficient embedded spirometer
    
    This README provides an overview of the embedded spirometer project, including objectives, system pipeline, firmware features, optimization focus, tech stack, and repository structure.

[33mcommit d5b81c0d0e7337f15308fd70a27540308433af3a[m
Author: LeninValentine06 <leninvalentine97@gmail.com>
Date:   Fri Jan 23 17:25:55 2026 +0530

    feat(spirometry): add Flow-Volume Loop (FVL) capture
    
    - record (volume, flow) pairs during forced exhalation
    - store samples in fixed-size buffer (800 points @ 10ms)
    - reset buffer at maneuver start
    - enable plotting of flow-volume curve for clinical analysis
    - foundation for FEF25-75 and waveform-based diagnostics

[33mcommit 60b753187ad3d88e1606177e6f934190ee94d440[m
Author: LeninValentine06 <leninvalentine97@gmail.com>
Date:   Fri Jan 23 16:42:09 2026 +0530

    feat(spirometry): add FVC, FEV1/FVC ratio and PEF computation
    
    - detect forced exhalation start using flow threshold
    - track maximum volume to compute FVC
    - compute FEV1/FVC ratio after maneuver completion
    - capture peak expiratory flow (PEF)
    - add hysteresis-based end-of-exhale detection
    - improves clinical spirometry metrics and reliability

[33mcommit 0d4b47135353d83f0ecd6ecafeda6e2ce5125cbb[m
Author: LeninValentine06 <leninvalentine97@gmail.com>
Date:   Fri Jan 23 16:21:02 2026 +0530

    Updated docs

[33mcommit 8040a6ff4f9903dbda86d96e68ff41a475eacd45[m
Author: LeninValentine06 <leninvalentine97@gmail.com>
Date:   Fri Jan 23 15:59:49 2026 +0530

    updated docs

[33mcommit 3e1587fd92471fb1e50939925fdeec1e6f4ce01b[m
Author: LeninValentine06 <leninvalentine97@gmail.com>
Date:   Fri Jan 23 15:59:00 2026 +0530

    feat(integration): replace Euler with trapezoidal integration
    
    - added trapezoidal integrator
    - added startup zero calibration
    - added deadband filtering
    - improved flow-to-volume accuracy
    
    Fixes #1

[33mcommit 2b80121527a9f57714462f51aa02a1a5ccb938d1[m
Author: LeninValentine06 <leninvalentine97@gmail.com>
Date:   Fri Jan 23 12:09:34 2026 +0530

    updated docs

[33mcommit dd857d97ead3e64e9df31cb84cbf5b294c8b78ab[m
Author: LeninValentine06 <leninvalentine97@gmail.com>
Date:   Fri Jan 23 12:05:47 2026 +0530

    removed old spirometer codebase

[33mcommit d1b6b8fbe8779c796f276c4a71826094c6b3688f[m
Author: LeninValentine06 <leninvalentine97@gmail.com>
Date:   Fri Jan 16 17:49:52 2026 +0530

    TODO: Volume caculation

[33mcommit 5ae175368d0c35ffce9fa3d20ce4ecdca153bee9[m
Author: LeninValentine06 <leninvalentine97@gmail.com>
Date:   Fri Jan 16 17:48:30 2026 +0530

    added: simple volum calculation

[33mcommit 887401778f0ecf727e866e41532f55e1f8a8d339[m
Author: LeninValentine06 <leninvalentine97@gmail.com>
Date:   Fri Jan 16 16:12:56 2026 +0530

    added: flow_lps calculation

[33mcommit 6e7279d8b63615097326caa580c58a5282b4dccc[m
Author: LeninValentine06 <leninvalentine97@gmail.com>
Date:   Fri Jan 16 16:05:43 2026 +0530

    chore: ignore STM32CubeIDE generated files

[33mcommit ca75352235f5802aa7aa98c3bccdf5d7ff52ad54[m
Author: LeninValentine06 <leninvalentine97@gmail.com>
Date:   Fri Jan 16 16:03:28 2026 +0530

    fix: enable continuous ADC conversion mode

[33mcommit 9d5fb264220daf5161a1afd731994c263b9cdc70[m[33m ([m[1;31morigin/test/flow[m[33m)[m
Author: LeninValentine06 <leninvalentine97@gmail.com>
Date:   Tue Jan 13 23:37:04 2026 +0530

    chore: remove obsolete STM32F401XE startup and linker files

[33mcommit c690f4177f527458dcd4839b6b2710552697d3e4[m
Author: LeninValentine06 <leninvalentine97@gmail.com>
Date:   Tue Jan 13 23:36:10 2026 +0530

    feat: update firmware sources after CubeIDE migration

[33mcommit 050beebd8863748d5d93c180ec436c4e8995f4a0[m
Author: LeninValentine06 <leninvalentine97@gmail.com>
Date:   Tue Jan 13 23:36:06 2026 +0530

    chore: remove CMake and VSCode configuration

[33mcommit a7b2a702296a9c5924cc825b3512f9334ecde0d9[m
Author: LeninValentine06 <leninvalentine97@gmail.com>
Date:   Tue Jan 13 23:36:02 2026 +0530

    build: migrate project to STM32CubeIDE (STM32F401CC)

[33mcommit 745d62b33136504e15f4817f68188baf5bc017f3[m
Author: LeninValentine06 <leninvalentine97@gmail.com>
Date:   Tue Jan 13 23:35:57 2026 +0530

    docs: add spirometer firmware documentation

[33mcommit ac888159796aed976509953c4f5a0203bade291e[m
Author: LeninValentine06 <leninvalentine97@gmail.com>
Date:   Sat Jan 10 10:35:04 2026 +0530

    testing flow

[33mcommit 265717ae348c013f7bce2649b9412f1a60c5fc47[m
Author: LeninValentine06 <leninvalentine97@gmail.com>
Date:   Sat Jan 10 00:39:05 2026 +0530

    Updated Docs

[33mcommit 0a5d15dc8cfa72f159219039aeeae4f9a00dd38a[m
Author: LeninValentine06 <leninvalentine97@gmail.com>
Date:   Fri Jan 9 22:53:40 2026 +0530

    Ignore Obsidian workspace files

[33mcommit 9c3c9c06bd672ad0b9ccb8fd7d024e7ddb57324b[m
Author: LeninValentine06 <leninvalentine97@gmail.com>
Date:   Fri Jan 9 22:36:53 2026 +0530

    Updated Spirometer documentation

[33mcommit 8c9ebaa8a25e28f252d501df0997140de85c39ed[m
Author: LeninValentine06 <leninvalentine97@gmail.com>
Date:   Fri Jan 9 12:28:53 2026 +0530

    added Docs

[33mcommit b6478b2d395a2e66f77d614c4bb2508f833b50c0[m
Author: LeninValentine06 <leninvalentine97@gmail.com>
Date:   Fri Jan 9 00:53:36 2026 +0530

    added documents

[33mcommit 742fad09cf49f85baa801e8376652affff106951[m
Author: LeninValentine06 <leninvalentine97@gmail.com>
Date:   Wed Jan 7 00:52:59 2026 +0530

    added fev1, fev6, fvc, fev1/fvc and pef

[33mcommit 8b9d415d508a42cbe8a9d15951bc1b2e8ee887b2[m
Author: LeninValentine06 <leninvalentine97@gmail.com>
Date:   Tue Jan 6 01:00:16 2026 +0530

    Initial commit for UROP
