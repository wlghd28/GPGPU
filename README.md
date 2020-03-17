# GPGPU
GPGPU에 대한 프로젝트 입니다.

# OpenCL 튜토리얼 링크
[1단계][https://hoororyn.tistory.com/1?category=712666]

[2단계][https://hoororyn.tistory.com/3?category=712666]

[3단계][https://hoororyn.tistory.com/4?category=712666]

[4단계][https://hoororyn.tistory.com/6?category=712666]

[5단계][https://hoororyn.tistory.com/7?category=712666]

[6단계][https://hoororyn.tistory.com/8?category=712666]

[7단계][https://hoororyn.tistory.com/10?category=712666]

[8단계][https://hoororyn.tistory.com/11?category=712666]

[9단계][https://hoororyn.tistory.com/12?category=712666]

# Local Memory와 work-group간의 동기화
[https://github.com/mjaysonnn/Accelerator/wiki/%EB%A1%9C%EC%BB%AC-%EB%A9%94%EB%AA%A8%EB%A6%AC%EC%99%80-%EC%9B%8C%ED%81%AC-%EA%B7%B8%EB%A3%B9%EA%B0%84%EC%9D%98-%EB%8F%99%EA%B8%B0%ED%99%94]

# 병렬처리의 기초 Part 1
[https://github.com/mjaysonnn/Accelerator/wiki/%EB%B3%91%EB%A0%AC%EC%B2%98%EB%A6%AC%EC%9D%98-%EA%B8%B0%EC%B4%88-Part-1]

# 병렬처리의 기초 Part 2
[https://github.com/mjaysonnn/Accelerator/wiki/%EB%B3%91%EB%A0%AC%EC%B2%98%EB%A6%AC%EC%9D%98-%EA%B8%B0%EC%B4%88-Part-2]

# work-group과 work-item의 개념
OpenCL 프로그램 중 호스트(Host)에서 실행되는 호스트 프로그램은, 커널을 실행하기 위해 인덱스 공간을 정의하며, 워크 아이템(work-item)은 인덱스 공간의 각 포인트에 해당한다. 워크 그룹(work-group)은 여러 워크 아이템의 집합을 말하며 모든 워크 그룹은 동일한 크기를 갖는다. 즉, 인덱스 공간은 같은 크기의 워크 그룹으로 분할되며 각 워크 그룹은 커널이 실행되는 디바이스(Device) 내의 하나의 CU(Compute Unit)에서 실행된다.

이때 워크 그룹의 크기는 보통 프로그래머가 호스트 코드에 직접 기입하는 방식으로 결정되며, OpenCL 런타임(run-time)이 이를 임의로 결정하게 하도록 하기 위해서는 NULL 값을 사용한다. OpenCL 커널에서 워크 그룹의 크기는 디바이스의 리소스 사용량을 결정하고 CU 사이의 로드 밸런스를 결정하기 때문에 성능에 중요한 영향을 미치는 요소이다.

# Local Memory
Compute Unit(CU)마다 local memory가 하나씩 있다. 해당 CU에서만 접근이 가능한 메모리이다. GPU의 경우, global memory에 접근하는 것보다 빠르다.
- global memory : GPU의 off-chip memory
- local memory : GPU SM 안의 Shared context

kernel 함수에서 할당해야 한다.
- kernel 함수 안에서 __local을 붙여 변수/배열을 선언한다.
- kernel 함수에서 __local 포인터를 인자로 받는다.
- sizeof(int) * 64 = 256Byte로 할당해야 한다.
- 할당과 동시에 초기화를 할 수 없다. kernel 실행 중에 따로 초기화를 해주어야 한다.(global memory변수는 선언과 동시에 초기화를 해주어야 한다.)

Global memory의 캐시(cache) 역할을 한다.
- global memory의 같은 위치를 여러 work-item이 읽을 경우 처음 한 번만 읽어서 local memory에 저장한다.

# OpenCL Memory Consistency
메모리의 내용이 서로 다른 work-item에게 항상 같게 보일 필요는 없기 때문에 위의 상황에서 문제가 발생한다.
이 문제를 해결하기 위해 동기화(synchronization)을 사용해야 한다.
- work-group barrier
- atomic operation

# work-group Barrier
kernel에서 barrier() 함수를 이용한다.
- void barrier(cl_mem_fence_flags flags)
- work-group 안의 모든 work-item이 barrier를 동시에 통과한다. 모든 work-item이 barrier까지 코드를 실행 후에 그 다음 코드를 실행한다.
- local memory에 대한 consistency 보장 : barrier(CLK_LOCAL_MEM_FENCE)
- global memory에 대한 consistency 보장 : barrier(CLK_GLOBAL_MEM_FENCE)
서로 다른 work-group 간의 동기화는 지원하지 않는다.
work-group 안의 모든 work-item이 barrier()를 실행하거나, 전체가 실행하지 않아야 한다.(if문 분기점 조심할 것)

# work-group 사이즈 결정
OpenCL을 사용하여 워크그룹 사이즈(CUDA에서는 Dimension, block, thread)를 결정해야 할때 Global workgorup size 는 해결하고자 하는 총 문제수를 커버 해야한다. 

즉, 1024 x 1024 행렬이 있다면 커맨드큐에 NDRagneKernel을 할때 global workgroup size는 1024 x 1024 = 1048576 이 된다.

Local workgorup size를 NULL 로 입력해주면 OpenCL이 알아서 실행하여 준다.

하지만 성능향상을 위해서는 local workgorup size를 수동으로 넣어주어야 할때가 있다.

연산장치(GPU 또는 기타장치)의 프로필을 보면 Max work group size 정보를 알 수 있다.

예를 들어 Max work group size 가 256 이었다면 local work group size는 모든 Dimenstion을 곱했을때 256 보다 같거나 작아야 한다.

저 256이라는 숫자는 보통 FrontWave(CUDA에서는 WARP) 에 의해서 결정되는데 Hardware 적으로 context switching 할 수 있는 총 그룹수와 PE(processing element) 수에 의해 결정된다. PE가 64개 이고 FraoneWawe가 4이면 256이기 때문에 local workgorup 에서 처리할수 있는 work item 갯수는 256개가 된다.

총연산 유닛(compute unit)이 32개 라면 64(PE)*32(CU) = 2048 개가 동시에 처리되고 2048*4 = 8192 개 의 work-item이 스위칭되면서 실행된다. 

이때 work-item들이 메모리에 동시에 접근할때 대역폭 정보 등까지 고려되어야 성능 향상에 대한 이해와 최적화를 할 수 있다.