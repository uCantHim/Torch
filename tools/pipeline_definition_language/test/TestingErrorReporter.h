#include <vector>

#include "ErrorReporter.h"

class TestingErrorReporter : public ErrorReporter
{
public:
    void reportError(const Error& error) override {
        errors.emplace_back(error);
    }

    auto getErrors() const -> const std::vector<Error>& {
        return errors;
    }

private:
    std::vector<Error> errors;
};
