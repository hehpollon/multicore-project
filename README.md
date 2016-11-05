Simple MVCC 와 Garvage Collector
============
****
## 1. 소개
### 1.1 대략적인 진행
해당 프로그램은 대략적으로 다음과 같이 동작합니다. 
1. 각 스레드마다 A, B , version을 가지는 노드를 가지는 링크리스트를 가지고 최초실행시 랜덤으로 A와 B의 값을 설정해주고 version은 0으로 초기화해줍니다. 그리고 각각의 스레드들의 업데이트를 실행시키고 한 업데이트가 끝날때마다 version order를 atomic하게 증가시

2. 각각의 스레드 x는 다른 스레드 i를 참조하여 해당 스레드의 가장 최신 버전의 Ai값을 가져와서 내 스레드인 스레드 x의 A와B에 Ai를 더하고 빼는식으로 그리고 atomic한 version order를 함께 스레드 x의 링크리스트에 업데이트를 해줍니다.

3. 이를 duration동안 지속적으로 반복합니다.
- - -

### 1.2 프로그램이 Correctness하게 동작하기 위해 해야할 것들.
모든 스레드들이 동시에 돌면서 서로서로의 스레드를 참조해가며 업데이트를 해가기에 충돌이 발생합니다.
이를 위해서 다음과 같은 기법이 필요합니다.

스레드 시작
- 참조할 스레드 i를 선정함.

--------------임계영역 진입-----------------

- 글로벌 active리스트에 내 스레드 index와 버전을 삽입하고 현재 내가 업데이트중임을 알려줌.
- 글로벌 액티브리스트를 캡쳐

--------------임계영역 탈출-----------------

- 캡쳐된 리스트에 스레드i가 존재한다면, 거기 있는 버전보다 작은 것을 가져와서 업데이트를 진행하고 아니라면 그냥 업데이트 진행

--------------임계영역 진입-----------------

- 글로벌 active리스트에 내 스레드 index를 가진 노드를 제거함.

--------------임계영역 탈출-----------------

이를 통해서 여러 스레드들이 동시에 Read와 Write를 진행하더라도 충돌을 방지 할 수 있습니다.

### 1.3 기타 유의할 것들.
1. 다음과 같은 3개의 글로벌 플래그를 사용하여 verbose와 duration동안 스레드가 진행되는 것을 조정합니다.

` 
//이 플래그 true일때만 각 스레드들이 update를 진행할 수 있습니다. (duration동안 동작하도록 하기 위해 존재합니다.
volatile bool g_could_thread_run = true;
//이 플래그가 true라면 verbose옵션이 적용됩니다.
volatile bool g_verbose_flag = false;
//verbose옵션으로 인해 텍스트파일을 만드는 동안 다른 스레드들의 동작을 정지시킵니다.
volatile bool g_is_verbosing = false;
`

2. 싱글리스트에 넣는 것을 역순으로 넣어서 성능을 향상시켰습니다. 이를 원래대로 넣을경우 약 10배가까이 성능의 차이가 있었습니다.

### 1.4 Garvage콜렉터와 락이 구현되지 않았을 경우의 결과 사진
![verbose_1](./images/verbose_1.PNG)
![verbose_1](./images/verbose_2.PNG)
verbose에 찍힌 값들은 사이즈는 점점 커져가고 가장 최신으로 업데이트된 값은 점차 증가하며 버전들은 모든 스레드에서 중복됨이 없이 점점 감소되면서 나타나게 됩니다.

* * *

## 2. Garvage Collector