#include "uWS/uWS.h"
#include <iostream>
#include "json.hpp"
#include "PID.h"
#include <math.h>

// for convenience
using json = nlohmann::json;

// For converting back and forth between radians and degrees.
constexpr double pi() { return M_PI; }
double deg2rad(double x) { return x * pi() / 180; }
double rad2deg(double x) { return x * 180 / pi(); }

// Checks if the SocketIO event has JSON data.
// If there is data the JSON object in string format will be returned,
// else the empty string "" will be returned.
std::string hasData(std::string s) {
  auto found_null = s.find("null");
  auto b1 = s.find_first_of("[");
  auto b2 = s.find_last_of("]");
  if (found_null != std::string::npos) {
    return "";
  }
  else if (b1 != std::string::npos && b2 != std::string::npos) {
    return s.substr(b1, b2 - b1 + 1);
  }
  return "";
}

int main()
{
  uWS::Hub h;

  PID steer_pid;
  PID speed_pid;
  // TODO: Initialize the pid variable.
//  steer_pid.Init(0.2, 0.3, 0.004);    //Starting coefficients
  steer_pid.Init(1.288, 5.318, 9.6e-5);    //Twiddled coefficients
  speed_pid.Init(0.1, 0.01, 0.0);

  h.onMessage([&steer_pid, &speed_pid](uWS::WebSocket<uWS::SERVER> ws, char *data, size_t length, uWS::OpCode opCode) {
    // "42" at the start of the message means there's a websocket message event.
    // The 4 signifies a websocket message
    // The 2 signifies a websocket event
    if (length && length > 2 && data[0] == '4' && data[1] == '2')
    {
      auto s = hasData(std::string(data).substr(0, length));
      if (s != "") {
        auto j = json::parse(s);
        std::string event = j[0].get<std::string>();
        if (event == "telemetry") {
          // j[1] is the data JSON object
          double cte = std::stod(j[1]["cte"].get<std::string>());
          double speed = std::stod(j[1]["speed"].get<std::string>());
          double angle = std::stod(j[1]["steering_angle"].get<std::string>());
          double steer_value, speed_ideal, throttle, time;
          bool training = false;
          /*
          * TODO: Calcuate steering value here, remember the steering value is
          * [-1, 1].
          * NOTE: Feel free to play around with the throttle and speed. Maybe use
          * another PID controller to control the speed!
          */

          // Steering controller
          steer_pid.UpdateError(cte);
          // Reset Simulator and controllers if error is high
          if (steer_pid.counter > steer_pid.training_cycle && steer_pid.sum_dp > 1e-5 && training) {
            std::string msg = "42[\"reset\",{}]";
            ws.send(msg.data(), msg.length(), uWS::OpCode::TEXT);
            steer_pid.Twiddle();
            std::cout << "------------------------------\n";
            std::cout << "Steer Kp: " << steer_pid.Kp << " Kd: " << steer_pid.Kd << " Ki: " << steer_pid.Ki << std::endl;
            std::cout << "Steering best error: " << steer_pid.best_err << std::endl;
            steer_pid.Init(steer_pid.Kp, steer_pid.Kd, steer_pid.Ki);
            speed_pid.isInitialised = false;
            speed_pid.Init(0.1, 0.01, 0.0);
          }
          // Get the controller error
          steer_value = steer_pid.TotalError();
            
          // Speed controller
          speed_ideal = speed_pid.IdealSpeed(steer_value);
          speed_pid.UpdateError(speed - speed_ideal);
//          speed_pid.Twiddle();
//          std::cout << "Speed Kp: " << speed_pid.Kp << " Kd: " << speed_pid.Kd << " Ki: " << speed_pid.Ki << std::endl;
          throttle = speed_pid.TotalError();

          // DEBUG
//          std::cout << "target speed = " << speed_ideal << " counter " << speed_pid.counter << std::endl;
//          std::cout << "Steer Kp: " << steer_pid.Kp << " Kd: " << steer_pid.Kd << " Ki: " << steer_pid.Ki << std::endl;
//          std::cout << "Speed Kp: " << speed_pid.Kp << " Kd: " << speed_pid.Kd << " Ki: " << speed_pid.Ki << std::endl;
//          std::cout << "CTE: " << cte << std::endl;
//          std::cout << "Steering best error: " << steer_pid.best_err << std::endl;

          json msgJson;
          msgJson["steering_angle"] = steer_value;
          msgJson["throttle"] = throttle;
          auto msg = "42[\"steer\"," + msgJson.dump() + "]";
//          std::cout << msg << std::endl;
          ws.send(msg.data(), msg.length(), uWS::OpCode::TEXT);
        }
      } else {
        // Manual driving
        std::string msg = "42[\"manual\",{}]";
        ws.send(msg.data(), msg.length(), uWS::OpCode::TEXT);
      }
    }
  });

  // We don't need this since we're not using HTTP but if it's removed the program
  // doesn't compile :-(
  h.onHttpRequest([](uWS::HttpResponse *res, uWS::HttpRequest req, char *data, size_t, size_t) {
    const std::string s = "<h1>Hello world!</h1>";
    if (req.getUrl().valueLength == 1)
    {
      res->end(s.data(), s.length());
    }
    else
    {
      // i guess this should be done more gracefully?
      res->end(nullptr, 0);
    }
  });

  h.onConnection([&h](uWS::WebSocket<uWS::SERVER> ws, uWS::HttpRequest req) {
    std::cout << "Connected!!!" << std::endl;
  });

  h.onDisconnection([&h](uWS::WebSocket<uWS::SERVER> ws, int code, char *message, size_t length) {
    ws.close();
    std::cout << "Disconnected" << std::endl;
  });

  int port = 4567;
  if (h.listen(port))
  {
    std::cout << "Listening to port " << port << std::endl;
  }
  else
  {
    std::cerr << "Failed to listen to port" << std::endl;
    return -1;
  }
  h.run();
}
