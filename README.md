DANBI: Dataflow Parallel Runtime for Manycore Systems
=====================================================

The stream programming model has received much interest because it naturally exposes task, data, and pipeline parallelism. However, most prior work has focused on the static scheduling of regular stream programs. Therefore, irregular applications cannot be handled in static scheduling, and the load imbalance caused by static scheduling faces scalability limitations in many-core systems. In this paper, we introduce the DANBI programming model, which supports irregular stream programs, and propose dynamic scheduling techniques. Scheduling irregular stream programs is very challenging, and the load imbalance becomes a major hurdle to achieving scalability. Our dynamic load-balancing scheduler exploits producer-consumer relationships already expressed in the DANBI program to achieve scalability. Moreover, it effectively avoids the thundering-herd problem and dynamically adapts to load imbalance in a probabilistic manner. It surpasses prior static stream scheduling approaches which are vulnerable to load imbalance and also surpasses prior dynamic stream scheduling approaches which result in many restrictions on supported program types, on the scope of dynamic scheduling, and on data ordering preservation. Our experimental results on a 40-core server show that DANBI achieves an almost linear scalability and outperforms state-of-the-art parallel runtimes by up to 2.8 times.


Author
-------
- Changwoo Min <changwoo@vt.edu>


Publications
------------
- Paper on DANBI
```
Dynamic Scheduling of Irregular Stream Programs toward Many-Core Scalability
Changwoo Min and Young Ik Eom
IEEE TPDS 2015

DANBI: Dynamic Scheduling of Irregular Stream Programs for Many-Core Systems
Changwoo Min and Young Ik Eom
PACT 2013
```
