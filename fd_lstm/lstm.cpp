#include "lstm.h"

#include <torch/torch.h>

// Define the LSTM model
struct LSTMModel : torch::nn::Module {
    LSTMModel(int inputSize, int hiddenSize, int outputSize)
        : lstm(torch::nn::LSTMOptions(inputSize, hiddenSize).batch_first(true)),
          fc(outputSize) {
        register_module("lstm", lstm);
        register_module("fc", fc);
    }

    torch::Tensor forward(torch::Tensor x) {
        torch::Tensor output, hiddenState;
        std::tie(output, hiddenState) = lstm(x);
        output = fc(output[:, -1, :]);
        return output;
    }

    torch::nn::LSTM lstm;
    torch::nn::Linear fc;
};

// Function to train the LSTM model
void trainModel(LSTMModel& model, torch::Device device, torch::nn::L1Loss lossFunc,
                torch::optim::Adam optimizer, torch::Tensor trainX, torch::Tensor trainY, int epochs) {
    model->train();
    model->to(device);
    trainX = trainX.to(device);
    trainY = trainY.to(device);

    for (int epoch = 0; epoch < epochs; ++epoch) {
        optimizer.zero_grad();
        auto output = model(trainX);
        auto loss = lossFunc(output, trainY);
        loss.backward();
        optimizer.step();
    }
}

// this give additional multiplier for 
struct CustomLoss : torch::nn::Module {
  CustomLoss() {}

  torch::nn::MSELoss lossFn;
  torch::Tensor loss

  torch::Tensor forward(torch::Tensor predicted, torch::Tensor target) {
    int predicted_val = predicted.item<int>();
    int target = predicted.item<int>();
    
    if (predicted > target){
        loss = lossFn(predicted, target);
        
    }
    return loss;
  }
};


// Function to make predictions using the LSTM model
torch::Tensor predict(LSTMModel& model, torch::Device device, torch::Tensor testX) {
    model->eval();
    model->to(device);
    testX = testX.to(device);
    auto output = model(testX);
    return output;
}

int main() {

    // Convert data to tensors
    torch::Tensor trainX = torch::from_blob(trainXData, {trainSize, 1, lookBack}, torch::kFloat32);
    torch::Tensor trainY = torch::from_blob(trainYData, {trainSize, 1}, torch::kFloat32);
    torch::Tensor testX = torch::from_blob(testXData, {testSize, 1, lookBack}, torch::kFloat32);

    // Define the model and other parameters
    int inputSize = 1;
    int hiddenSize = 4;
    int outputSize = 1;
    LSTMModel model(inputSize, hiddenSize, outputSize);
    torch::nn::L1Loss lossFunc;
    torch::optim::Adam optimizer(model.parameters(), torch::optim::AdamOptions(0.001));

    // Train the model
    int epochs = 100;
    trainModel(model, device, lossFunc, optimizer, trainX, trainY, epochs);

    // Make predictions
    torch::Tensor predictions = predict(model, device, testX);

    
    return 0;
}
