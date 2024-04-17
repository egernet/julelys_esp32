#ifndef JS_CONTROLLER_H
#define JS_CONTROLLER_H

class LedController;

class JSController {
private:
    bool stop;
    struct js *jsEngine;

    struct js *setup(double matrixHeight, double matrixWidth, long start, long time);
public:
    JSController(LedController *controller);

    void runCode(const char *code);
    void runCodeOne(const char *code);

    void stopRinning();
};

#endif /* JS_CONTROLLER_H */
