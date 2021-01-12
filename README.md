---
tags: 平行程式設計
---

# SIMD Programming
<font color="red">此作業是模擬SIMD指令，並非使用一般常見的SSE, AVX等...</

Part1
---
* Q1-1
    >Implement a vectorized version of clampedExpSerial in clampedExpVector (using fake vector intrinsics). Your implementation should work with any combination of input array size (N) and vector width (VECTOR_WIDTH), achieve a vector utilization higher than 60%, and of course pass the verification. (You can assume the array size is much bigger than the vector width.)

    >Run ./myexp -s 10000 and sweep the vector width from 2, 4, 8, to 16. Record the resulting vector utilization. You can do this by changing the #define VECTOR_WIDTH value in def.h. Q1-1: Does the vector utilization increase, decrease or stay the same as VECTOR_WIDTH changes? Why?
     
     
    在N=10000時，各個Vector Width的執行結果如下
    * Vector Width: 2
        Vector Utilization: 86.5%
        ![](https://i.imgur.com/JFcDZWo.png)


    * Vector Width: 4
        Vector Utilization: 82.4%
        ![](https://i.imgur.com/tuiis4r.png)


    * Vector Width: 8
        Vector Utilization: 80.4%
        ![](https://i.imgur.com/rJJeb0y.png)


    * Vector Width: 16
        Vector Utilization: 79.4%
        ![](https://i.imgur.com/sQiOXdD.png)


    
    Vector Utilization會隨著Vector Width的上升而**下降**。
    下降的原因分為**兩種**:
    1. **切割資料**
        ```cpp
        for (int i = 0; i < N; i += VECTOR_WIDTH)
        ```
        以上的迴圈計算為例，
        若:
        * VECTOR_WIDTH = 4, 
        * N = 13
        
        則計算的時候會切割資料為以下幾組(數字代表index)
         [0 ~ 3], [4 ~7], [8 ~ 11], [12]
        
        在最後一組中只有一個資料被利用到，但VECTOR_WIDTH是切割為4的空間，所以有3個空間在計算上沒被使用到，造成浪費。
        當N被放大時，只要不是VECTOR_WIDTH的倍數就會造成空間浪費，且當VECTOR_WIDTH放大數倍後(如: VECTOR_WIDTH = 12, N = 13)在切割上勢必會比較小的VECTOR_WIDTH造成更多的空間浪費，也因此降低了Vector Utilization。
        
    2. **指數運算**
        在指數運算時需要將exponents為0的lane做mask，因為只剩下exponents還有值的value需要計算，其他的都已計算完畢，所以會造成空間浪費。
        
        以下圖為例:
        | Index    |0  |1  |2  |3  |
        | -----    |-- |-- |-- |-- |
        | Value    |1.1|1.2|1.3|1.4|
        | Exponents| 1 | 0 | 0 | 0 |
        VECTOR_WIDTH = 4
        
        Index為 1, 2, 3的資料exponents為0，所以不需要進入指數運算相乘的環節，而Index為0的資料需要經過1次的乘法。
        因為VECTOR_WIDTH = 4，所以每次計算時都是4個4個一起，mask會設定為
        
        | Index    |0  |1  |2  |3  |
        | -----    |-- |-- |-- |-- |
        | Mask     | 1 | 0 | 0 | 0 |
        
        因為Index為 1, 2, 3的資料不須計算所以Mask設定為0。從Mask可以看出這這次的計算中浪費了3個計算空間，若當VECTOR_WIDTH擴大時，則最後一次的指數計算所造成的空間浪費以機率而言會比小的VECTOR_WIDTH還大(最多會浪費掉VECTOR_WIDTH - 1的計算空間)。
        
        舉個簡單的例子:
        ***以下簡稱VECTOR_WIDTH=16的Vector為V16，VECTOR_WIDTH=4的Vector為V4。***
        1個V16的大小等於4個V4。
        4個V4都在最壞的指數數值分配的結果為1個9配3個0
        如:
        ```
        第一個V4   第二個V4   第三個V4   第四個V4
        9000       0900      0090      00009
        
        #註: 數字代表該位置的數值所對應的Exponents
        ```
        而一個V16最壞的結果為1個9配15個0，所以以機率來看V16有機會比4個V4的Vector Utilization還要差。
        
        若以相同的指數分配來做比較
        如:
        ```
        V16
        9000 0000 0000 0000  需要進行9次的指數運算，浪費了15*9的計算空間
        
        一樣的結果轉為V4
        第一個V4   第二個V4   第三個V4   第四個V4
        9000       0000      0000      0000   只有第一個V4需要進行9次指數運算
                                              相對於V16只浪費了3*9的計算空間
        
        ```
        V4, V16都是分配相同的指數數值，但V4的Vector Utilization顯著比V16還要有效率。
        
        
Part 2
---
* Q2-1
     > Fix the code to make sure it uses aligned moves for the best performance.
 
    將
    ```cpp
    __builtin_assume_aligned(*, 16);
    ```
    改為
    ```cpp
    __builtin_assume_aligned(*, 32);
    ```
    ![](https://i.imgur.com/NxHWx6p.png)
    ![](https://i.imgur.com/KsbEyPn.png)
    即可看見vmovaps 而不是vmovups
    
* Q2-2
    > What speedup does the vectorized code achieve over the unvectorized code? What additional speedup does using -mavx2 give (AVX2=1 in the Makefile)? You may wish to run this experiment several times and take median elapsed times; you can report answers to the nearest 100% (e.g., 2×, 3×, etc). What can you infer about the bit width of the default vector registers on the PP machines? What about the bit width of the AVX2 vector registers.


    執行
    * case 1
        ```cpp
        $ make clean && make && ./test_auto_vectorize -t 1
        ```
        結果為
        |  次數| 執行時間(秒)| 
        | --- | --------  |
        |1    | 8.17017 |
        |2    | 8.171   |
        |3    | 8.17114 |
        |4    | 8.17046 |
        |5    | 8.17089 |

        執行時間約為: 8.17秒
    
    * case 2
        ```cpp
        $ make clean && make VECTORIZE=1 && ./test_auto_vectorize -t 1
        ```
        結果為
        |  次數| 執行時間(秒)| 
        | --- | --------  |
        |1    | 2.60918 |
        |2    | 2.60964 |
        |3    | 2.61055 |
        |4    | 2.61045 |
        |5    | 2.61019 |

        執行時間約為: 2.61秒

    * case 3
        ```cpp
        $ make clean && make VECTORIZE=1 AVX2=1 && ./test_auto_vectorize -t 1
        ```
        
        結果為
        |  次數| 執行時間(秒)| 
        | --- | --------  |
        |1    | 1.35531 |
        |2    | 1.35359 |
        |3    | 1.35392 |
        |4    | 1.35336 |
        |5    | 1.35409 |

        執行時間約為: 1.35秒 
    
    case2 與 case3的時間差異約為兩倍的時間，而AVX2的vector registers為256bits(來源:https://en.wikipedia.org/wiki/Advanced_Vector_Extensions)，因此推測原本PP machines預設的vector registers為128bits(與AVX2時間差距約為大小差距)。
    作業的內容也提到，*clang has used packed SSE instructions to add 16 bytes at a time*，SSE一次處理正好為以上所推論的128bits(16bytes)。
    
* Q2-3
     > Provide a theory for why the compiler is generating dramatically different assembly.
    :::info
    推測:
    :::
    
    原本的寫法
    ```cpp
    for (int j = 0; j < N; j++)
    {
        c[j] = a[j];
        if (b[j] > a[j])
            c[j] = b[j];
    }
    ```
    在迴圈內還未做判斷時就已經有一次assign ```c[j] = a[j];```，所以在compilation階段compiler需要先做完這一步assign才進入判斷，讓向量化的處理被中斷。
    
    修改後的寫法
    ```cpp
    for (int j = 0; j < N; j++)
    {
        if (b[j] > a[j]) c[j] = b[j];
        else c[j] = a[j];
    }
    ```
    在迴圈內判斷完後才做assign，因此compiler可以利用向量化的優勢對if-else去做等效的處理。
    :::success
    理由:
    :::
    
    會這樣推斷的理由為，若將原本的程式碼改為
    * 第一點
        ```cpp
        for (int j = 0; j < N; j++)
        {
            if (b[j] > a[j])
                c[j] = b[j];
            c[j] = a[j];
        }
        ```
        雖然結果並不是我們要的，但在組合語言的部分卻可以明顯地看到**有向量化**的指令出現。
        ![](https://i.imgur.com/oktjsvm.png)

    * 第二點
        或是將改寫後的程式碼之前再加入assign的程式
        ```cpp
        for (int j = 0; j < N; j++)
        {
          c[j] = a[j];
          if (b[j] > a[j]) c[j] = b[j];
          else c[j] = a[j];
        }
        ```
        儘管有點多此一舉，但在編譯成組合語言後得到的結果會發現原本平行化的指令"movaps"會被別的指令取代掉，**失去了原本向量化的功能**。
        
    因此推斷出，若將assign的語句置於for迴圈內的第一行會破壞掉原本向量化的功能。
