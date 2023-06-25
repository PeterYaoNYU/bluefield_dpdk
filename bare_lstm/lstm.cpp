#include "lstm.h"

// sample input from a real fd application
// 1: 13771000146477305 | 2: 13771000346217979 | 3: 13771000545887755 | 4: 13771000745551751 | 5: 13771000945219877 |
//  6: 13771001144883721 | 7: 13771001344551759 | 8: 13771001544230683 | 9: 13771001743904147 | 10: 13771001943574147 

// the next arrival time 13771002143241345

LSTMModel::LSTMModel(int in_features, int hidden_layer_size, int out_size, int num_layers, bool batch_first){
    lstm = torch::nn::LSTM(lstmOption(in_features, hidden_layer_size, num_layers, batch_first));
    ln = torch::nn::Linear(hidden_layer_size, out_size);

    lstm = register_module("lstm",lstm);
    ln = register_module("ln",ln);
}

torch::Tensor LSTMModel::forward(torch::Tensor x){
    auto lstm_out = lstm->forward(x);
    auto predictions = ln->forward(std::get<0>(lstm_out));
    return predictions.select(1,-1);
}

// trainX is the training input
// trainY is the training output 
void LSTMModel::trainModel(LSTMModel& model, torch::Device device, torch::nn::MSELoss lossFunc,
                torch::optim::Adam& optimizer, torch::Tensor trainX, torch::Tensor trainY, int epochs)
{
    model.train();
    model.to(device);
    trainX = trainX.to(device);
    trainY = trainY.to(device);
    
    int epoch;
    for (epoch = 1; epoch <= epochs; epoch++){
        torch::Tensor output = model.forward(trainX);
        torch::Tensor loss = lossFunc(output, trainY);

        // backprop and oiptimize
        optimizer.zero_grad();
        loss.backward();
        optimizer.step();

        // print the progress
        if (epoch  % 5 == 0){
            std::cout << "Epoch: " << epoch << ", Loss: " << loss.item<float>() << std::endl;
        }
    }
}

torch::Tensor LSTMModel::predict(LSTMModel& model, torch::Tensor input) {
    model.eval(); // Set the model to evaluation mode

    // Forward pass
    torch::Tensor output = model.forward(input);

    model.train(); // Set the model back to training mode

    return output;
}

int main(){
    // number of features at a single timestamp
    // in the context of fd, it should be one single scalar valued timestamp
    int input_size = 1;
    int hidden_size = 4;
    int output_size = 1;
    int num_layers = 1;
    bool batch_first = true;
    int num_epochs = 100;
    int batch_size = 1;
    float learning_rate = 0.001;
    // sequence length refers to the length or number of time steps in a sequence.
    // It represents how far back in time the LSTM can look when processing the input sequence.
    int sequence_length = 3;

    // create an instance of the model 
    LSTMModel model(input_size, hidden_size, output_size, num_layers, batch_first);

    // define the loss anf optimization
    torch::nn::MSELoss criterion;
    torch::optim::Adam optimizer(model.parameters(), torch::optim::AdamOptions(learning_rate));

    std::vector<int64_t> values = {13771000146477305,
                                    13771000346217979,
                                    13771000545887755,
                                    13771000745551751,
                                    13771000945219877,
                                    13771001144883721,
                                    13771001344551759,
                                    13771001544230683,
                                    13771001743904147,
                                    13771001943574147};

    // Create a tensor from the vector of values
    torch::Tensor input = torch::tensor(values, torch::kInt64);

    // define some random inputs;
    input = input.view({batch_size, 10 , input_size});
    input = input.to(torch::kFloat32);

    std::cout << "Tensor: " << input << std::endl;
    torch::Tensor target = torch::tensor({13771002143241345}, torch::kInt64);
    target = target.view({batch_size, output_size});
    target = target.to(torch::kFloat32);

    std::cout << "Target: " << target << std::endl;


    // torch::Tensor input = torch::randn({batch_size, sequence_length, input_size});
    // torch::Tensor target = torch::randn({batch_size, output_size});

    // std::cout << "Tensor: " << input << std::endl;
    // std::cout << "Target: " << target << std::endl;


    model.trainModel(model, torch::kCPU, criterion, optimizer, input, target, num_epochs);

    // auto predicted = model.predict(model, target);
    // std::cout << predicted << std::endl;


    // making a simple prediction for the 12-th arrival 

}