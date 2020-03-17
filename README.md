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

# 로컬 메모리와 워크 그룹간의 동기화
[https://github.com/mjaysonnn/Accelerator/wiki/%EB%A1%9C%EC%BB%AC-%EB%A9%94%EB%AA%A8%EB%A6%AC%EC%99%80-%EC%9B%8C%ED%81%AC-%EA%B7%B8%EB%A3%B9%EA%B0%84%EC%9D%98-%EB%8F%99%EA%B8%B0%ED%99%94]

# 병렬처리의 기초 Part 1
[https://github.com/mjaysonnn/Accelerator/wiki/%EB%B3%91%EB%A0%AC%EC%B2%98%EB%A6%AC%EC%9D%98-%EA%B8%B0%EC%B4%88-Part-1]

# 병렬처리의 기초 Part 2
[https://github.com/mjaysonnn/Accelerator/wiki/%EB%B3%91%EB%A0%AC%EC%B2%98%EB%A6%AC%EC%9D%98-%EA%B8%B0%EC%B4%88-Part-2]

# 워크그룹과 워크아이템의 개념
OpenCL 프로그램 중 호스트(Host)에서 실행되는 호스트 프로그램은, 커널을 실행하기 위해 인덱스 공간을 정의하며, 워크 아이템(work-item)은 인덱스 공간의 각 포인트에 해당한다. 워크 그룹(work-group)은 여러 워크 아이템의 집합을 말하며 모든 워크 그룹은 동일한 크기를 갖는다. 즉, 인덱스 공간은 같은 크기의 워크 그룹으로 분할되며 각 워크 그룹은 커널이 실행되는 디바이스(Device) 내의 하나의 CU(Compute Unit)에서 실행된다.

이때 워크 그룹의 크기는 보통 프로그래머가 호스트 코드에 직접 기입하는 방식으로 결정되며, OpenCL 런타임(run-time)이 이를 임의로 결정하게 하도록 하기 위해서는 NULL 값을 사용한다. OpenCL 커널에서 워크 그룹의 크기는 디바이스의 리소스 사용량을 결정하고 CU 사이의 로드 밸런스를 결정하기 때문에 성능에 중요한 영향을 미치는 요소이다.