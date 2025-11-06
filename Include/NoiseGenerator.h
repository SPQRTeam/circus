#include <Eigen/Dense>
#include <random>

class NoiseGenerator
{
public:
    // Constructor to initialize noise generator with options
    NoiseGenerator(
        double normalNoiseStd = 0.0,
        double randomWalkStd = 0.0,
        const Eigen::VectorXd& initialBias = Eigen::VectorXd::Zero(3))
        : normalNoiseStd_(normalNoiseStd),
          randomWalkStd_(randomWalkStd),
          bias_(initialBias),
          dist_(0.0, 1.0),
          gen_(std::random_device{}())
    {
    }

    // Enable or disable different types of noise
    void enableNormalNoise(bool enable) { enableNormalNoise_ = enable; }
    void enableRandomWalk(bool enable) { enableRandomWalk_ = enable; }

    // Generic method for any Eigen vector-like type
    template <typename Derived>
    void addNoise(const Eigen::MatrixBase<Derived>& input,
                  Eigen::MatrixBase<Derived>& output)
    {
        assert(input.size() == bias_.size());
        if (output.size() != input.size())
            output = input; // resize if needed

        // Update the random walk bias first
        if (enableRandomWalk_) {
            for (int i = 0; i < bias_.size(); ++i) {
                bias_(i) += randomWalkStd_ * dist_(gen_);
            }
        }

        // Add noise
        for (int i = 0; i < input.size(); ++i) {
            double noise = 0.0;
            if (enableNormalNoise_) {
                noise += normalNoiseStd_ * dist_(gen_);
            }
            output(i) = input(i) + bias_(i) + noise;
        }
    }

    void setNormalNoiseStd(double std) { normalNoiseStd_ = std; }
    void setRandomWalkStd(double std) { randomWalkStd_ = std; }

private:
    bool enableNormalNoise_ = true;
    bool enableRandomWalk_ = true;

    double normalNoiseStd_;
    double randomWalkStd_;
    Eigen::VectorXd bias_;

    std::normal_distribution<double> dist_;
    std::mt19937 gen_;
};
