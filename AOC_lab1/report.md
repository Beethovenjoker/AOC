# AI Model Design and Quantization
## 1. Model Architecture (15%)
### Model
我是參考助教的模型架構下去改的，因為助教的模型參數量比較大，所以我將各層的通道數降低，以大幅減少模型參數，最終模型參數數量約為66萬個（2.52MB）。整體架構包含5個Convolution Block與3個Fully Conneted Block：
- Convolution Block
    - 使用Conv2d擷取影像特徵，並透過降低通道數(如32→64→128)來減少參數量。
    - 加入Batch Normalization提升訓練穩定性，加速收斂。
    - 使用ReLU激活函數增加非線性特徵的表達能力。
    - 每個區塊結尾搭配MaxPooling降低特徵圖維度，減少計算負擔。
- Fully Connected Block
    - 使用3層Linear層逐步降低特徵維度（128→64→10），以精簡參數數量並避免過擬合。
    - 層與層間亦使用ReLU維持非線性。

| Layer (type)      | Output Shape         | Param #   |
|-------------------|---------------------|-----------|
| Conv2d           | [-1, 32, 32, 32]     | 896       |
| BatchNorm2d      | [-1, 32, 32, 32]     | 64        |
| ReLU             | [-1, 32, 32, 32]     | 0         |
| MaxPool2d        | [-1, 32, 16, 16]     | 0         |
| Conv2d           | [-1, 64, 16, 16]     | 18,496    |
| BatchNorm2d      | [-1, 64, 16, 16]     | 128       |
| ReLU             | [-1, 64, 16, 16]     | 0         |
| MaxPool2d        | [-1, 64, 8, 8]       | 0         |
| Conv2d           | [-1, 128, 8, 8]      | 73,856    |
| BatchNorm2d      | [-1, 128, 8, 8]      | 256       |
| ReLU             | [-1, 128, 8, 8]      | 0         |
| Conv2d           | [-1, 128, 8, 8]      | 147,584   |
| BatchNorm2d      | [-1, 128, 8, 8]      | 256       |
| ReLU             | [-1, 128, 8, 8]      | 0         |
| Conv2d           | [-1, 128, 8, 8]      | 147,584   |
| BatchNorm2d      | [-1, 128, 8, 8]      | 256       |
| ReLU             | [-1, 128, 8, 8]      | 0         |
| MaxPool2d        | [-1, 128, 4, 4]      | 0         |
| Linear           | [-1, 128]            | 262,272   |
| ReLU             | [-1, 128]            | 0         |
| Linear           | [-1, 64]             | 8,256     |
| ReLU             | [-1, 64]             | 0         |
| Linear           | [-1, 10]             | 650       |

### Parameters
| Metric                          | Size        |
|---------------------------------|-------------|
| Total params                    | 660554      |
| Trainable params                | 660554      |
| Non-trainable params            | 0           |


### Size
| Metric                          | Size        |
|---------------------------------|-------------|
| Input size                      | 0.01 MB     |
| Forward/backward pass size      | 1.80 MB     |
| Params size                     | 2.52 MB     |
| Estimated Total Size            | 4.33 MB     |

## 2. Loss/Epoch and Accuract/Epoch Plotting (15%)
### Training and Validation Plots

![Loss and Accuracy Curve](../N26132314_lab1/image/Loss%20and%20Accuracy%20Curve.png)

### Test
| Metric       | Value    |
|-------------|---------|
| **Test Loss**    | 0.3696  |
| **Test Accuracy** | 0.8867  |
| **Model Size**    | 2.66 MB |
### Overfitting
此模型出現了明顯的overfitting。
- Loss
    - Training loss持續明顯下降，直到訓練結束仍保持下降趨勢。
    - Validation loss則約在第20個epoch後逐漸趨緩甚至穩定，不再明顯下降，且與training loss的差距逐漸拉大。
- Accuracy
    - Training accuracy持續上升，到最後epoch仍持續提升。
    - Validation accuracy約在第30~40個epoch後逐漸趨於穩定，且明顯低於training accuracy，兩者的差距逐漸擴大。
## 3. Accuracy Tuning (20%)
| **Hyperparameter**   | **Loss Function**       | **Optimizer**                                      | **Scheduler**                                      | **Weight Decay / Momentum**      | **Epoch**  |
|----------------------|------------------------|----------------------------------------------------|----------------------------------------------------|----------------------------------|------------|
| **Value**           | `CrossEntropyLoss()`    | `SGD(lr=0.1, momentum=0.9, weight_decay=5e-4)`    | `CosineAnnealingLR(T_max=epochs, eta_min=1e-6)`   | `weight_decay=5e-4, momentum=0.9` | `50`   |

### Loss function
`CrossEntropyLoss()`
- 由於本次作業是多類別分類問題，使用Cross Entropy Loss作為損失函數最合適，因為它能夠有效衡量模型預測的類別機率分布與真實標籤之間的差距。
### Optimizer
`SGD(lr=0.1, momentum=0.9, weight_decay=5e-4)`
- 選擇隨機梯度下降法（SGD），因為相比於Adam，SGD 通常能提供更好的泛化能力。
- 設定動量（Momentum=0.9）可加快模型收斂，減少訓練過程中的震盪，使參數更新更加平滑且快速。
- 加入權重衰減（Weight Decay=5e-4），具有 L2 正規化效果，抑制過大的參數值，避免模型Overfitting。
### Scheduler
`CosineAnnealingLR(T_max=epochs, eta_min=1e-6)`
- 採用餘弦退火（Cosine Annealing）學習率排程器，能以餘弦曲線逐步降低學習率，讓模型更穩定且高效地收斂。
- 設置 T_max=epochs 可確保學習率在整個訓練週期內平滑地下降至設定的最小值（eta_min=1e-6）。
### Weight Decay/Momentum
`weight_decay=5e-4, momentum=0.9`
- 權重衰減（weight_decay=5e-4）能夠有效控制模型的複雜度，防止過度擬合。
- 動量（momentum=0.9 ) 則幫助模型更快速、穩定地收斂，並有效避免陷入局部最小值。
### Epoch
`50`
- 經過測試，50 個訓練週期能讓模型有充足時間進行學習，同時也避免訓練過久而導致過度擬合。
## 4. Explain how the Power-of-Two Observer in your QConfig is implemented. (25%)

### Scale & Zero-point
由於weights通常分布於（-1, 1），因此使用 INT8 範圍（-128, 127）進行量化。而activation經 ReLU 後為介於（0, ∞），故使用 UINT8 範圍（0, 255）進行量化。
設輸入數據的範圍為 $[min_{value}, max_{value}]$：

- Weight 的 scale 與 zero-point 計算（INT8 對稱量化）
    - 設定 $r_{max} = \max(abs(min_{value}), abs(max_{value}))$，量化至 [-127, 127]。
    - 計算 zero-point：由於對稱量化，故 zero-point 為 0。
    - 計算scale:
$$
scale \geq \frac{2 \times r_{max}}{127}, \quad 
scale = 2^{\text{round}\left(\log_{2}\left(\frac{2 \times r_{max}}{127}\right)\right)}
$$


- Activation 的 scale 與 zero-point 計算（UINT8 對稱量化）
    - 直接使用 $\max(min_{value}, max_{value})$ 映射至 [0, 255]。
    - 計算 zero-point：由於對稱量化，故 zero-point 為 128。
    - 計算scale:
$$
scale \geq \frac{2 \times (max_{value}-min_{value})}{255}, \quad 
scale = 2^{\text{round}\left(\log_2\left(\frac{2 \times (max_{value}-min_{value})}{255}\right)\right)}
$$

- 計算量化後的數值
$$
q = \text{round}\left(\frac{x}{scale}\right) + zero\_point
$$

### scale_approximate()
設定的自訂 QConfig 如下：
```python
class CustomQConfig(Enum):
    POWER2 = tq.QConfig(
        activation=PowerOfTwoObserver.with_args(
            dtype=torch.quint8, qscheme=torch.per_tensor_symmetric
        ),
        weight=PowerOfTwoObserver.with_args(
            dtype=torch.qint8, qscheme=torch.per_tensor_symmetric
        ),
    )
    DEFAULT = None
```

函數 `scale_approximate()` 實作的步驟：
- 取得 input data 的最大最小值：
```python
min_val, max_val = self.min_val, self.max_val
```
- 根據資料型態決定量化範圍與 zero-point：

| 資料型態 | qmin | qmax | zero_point |
|----------|------|------|------------|
| INT8（Weight） | -127 | 127 | 0 |
| UINT8（Activation） | 0 | 255 | 128 |

- 計算 scale 並逼近至最接近的 2 的冪次方：
```python
max_abs = max(abs(min_val), abs(max_val))

if max_abs == 0:
    scale = 0
elif max_abs == float("inf"):
    scale = 1.0
else:
    scale = 2 * max_abs / qmax

scale = self.scale_approximate(scale)
scale = torch.tensor(scale, dtype=torch.float32)
```

### Overflow
在實作上，會遇到以下可能導致 Overflow 或數值異常的情況：
- 若輸入的資料範圍為 0（即 min_val == max_val），則會導致無法有效計算 scale，這時設定預設值：
```python
scale = 1.0
zero_point = 0
```
- 當 max_abs 為 0 或 inf 時，會造成計算異常，因此直接設定：
```python
if max_abs == 0:
    scale = 0
elif max_abs == float("inf"):
    scale = 1.0
```
- 使用 INT8 時，範圍應該設定為 [-127, 127] 而非 [-128, 127]，否則會因為 ∣−128∣>127而造成 Overflow。
- 計算 zero-point 時，透過四捨五入 round 並使用 torch.clamp 確保 zero-point 位於合法範圍 [qmin, qmax] 內：
```python
zero_point = torch.clamp(round(zero_point), qmin, qmax)
```
### Test (Quantized Model)
| Metric            | Value       |
|-------------------|-------------|
| **Test Loss**     | 0.3881      |
| **Test Accuracy** | 0.8800      |
| **Model Size**    | 0.673672 MB |
| **Accuracy Drop** | 0.67%       |

## 5. Comparison of Quantization Schemes (25%)

| Operation                        | Energy consumption (pJ)    |
| -------------------------------- | -------------------------- |
| FP32 Multiply                    | 3.7                        |
| FP32 Add                         | 0.9                        |
| <font color=red>INT32 Add</font> | <font color=red>0.1</font> |
| INT8 / UINT8 Multiply            | 0.2                        |
| INT8 / UINT8 Add                 | 0.03                       |
| Bit Shift                        | 0.01                       |


### Process
- Linear
    - Input size: 1x128
    - Output size: 1x10
#### Before quantization (FP32)
$$
y_i = \text{ReLU}(b_i + \sum_j x_j \cdot w_{ji})
$$

**Data type of FP32**:

|           | Activation | Weight | Bias  |  
|-----------|-----------|--------|-------|  
| **Data type** | FP32 | FP32  | FP32 |

$$
\begin{aligned}
\text{FP Energy Consumption} &= 10 \times \Biggl( 
\overbrace{(128 \times \text{FP32 Multiplication})}^{\text{Input tensor times weight}} \\
&\quad + \overbrace{(127 \times \text{FP32 Addition})}^{\text{Summation}} \\
&\quad + \overbrace{(1 \times \text{FP32 Addition})}^{\text{Add Bias}} \Biggr) \\ 
&= 5888 \text{ pJ}
\end{aligned}
$$

### After quantization
$$
\begin{aligned}
\bar y_i &\approx 2^{-(c_x + c_w - c_y)} \text{ReLU}(\bar b_i + \sum_j (\bar x_j - 128) \cdot \bar w_{ji}) + 128 \\
&= \left( \text{ReLU}(\bar b_i + \sum_j (\bar x_j - 128) \cdot \bar w_{ji}) \gg (c_x + c_w - c_y) \right) + 128
\end{aligned}
$$

**Data type of INT8**:

|           | Activation | Weight | Bias  |  
|-----------|-----------|--------|-------|  
| **Data type** | UINT8 | INT8  | INT32 |

$$
\begin{aligned}
\text{INT8 Energy Consumption} &= 10 \times \Biggl( 
\overbrace{(128 \times \text{INT8 Addition})}^{\text{subtract zero point}} \\
&\quad + \overbrace{(128 \times \text{INT8 Multiplication})}^{\text{Input tensor times weight}} \\
&\quad + \overbrace{(127 \times \text{INT32 Addition})}^{\text{Summation}} \\
&\quad + \overbrace{(1 \times \text{INT32 Addition})}^{\text{Add Bias}} \\
&\quad + \overbrace{(1 \times \text{Bit Shift})}^{\text{Power of Two Scale Shifting}} \\
&\quad + \overbrace{(1 \times \text{INT8 Addition})}^{\text{Add Zero Point}} \Biggr) \\ 
&= 422.8 \text{ pJ}
\end{aligned}
$$


### Your Answer

|                         | Before quantization (FP32) | After quantization |
| ----------------------- | ---- | --------- |
| Energy consumption (pJ) | 5888     | 422.8          |
## Reference
- [Overview of AOC Labs](https://hackmd.io/@yutingshih/aoc2025/%2FD2ydK5m2SzOHnY8uzZAH1Q)