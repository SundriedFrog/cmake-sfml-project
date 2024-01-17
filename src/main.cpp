#include <SFML/Graphics.hpp>
#include <vector>
#include <cmath>
#include <cstdlib> // For rand() and srand()
#include <ctime>   // For time()
#include <thread>
#include <windows.h>
#include <random>
#include <deque>                   // Include the necessary header file for std::deque
#include <SFML/System/Vector2.hpp> // Include the necessary header file for sf::Vector2f
#include <SFML/Audio.hpp>
#include <iostream>

class Ball : public sf::CircleShape
{
public:
    sf::Vector2f velocity;
    std::deque<sf::Vector2f> trail; // Fix the qualified name and missing type specifier

    Ball(float radius) : sf::CircleShape(radius)
    {
        velocity = sf::Vector2f(0, 0);
    }
};

sf::Color hsvToRgb(float h, float s, float v)
{
    int i = std::floor(h * 6);
    float f = h * 6 - i;
    float p = v * (1 - s);
    float q = v * (1 - f * s);
    float t = v * (1 - (1 - f) * s);

    switch (i % 6)
    {
    case 0:
        return sf::Color(v * 255, t * 255, p * 255);
    case 1:
        return sf::Color(q * 255, v * 255, p * 255);
    case 2:
        return sf::Color(p * 255, v * 255, t * 255);
    case 3:
        return sf::Color(p * 255, q * 255, v * 255);
    case 4:
        return sf::Color(t * 255, p * 255, v * 255);
    default:
        return sf::Color(v * 255, p * 255, q * 255);
    }
}

void verlet(Ball &ball, float deltaTime, sf::FloatRect boundary)
{
    // Define the friction factor
    float friction = 0.01f;

    sf::Vector2f pos = ball.getPosition();
    sf::Vector2f center = pos + sf::Vector2f(ball.getRadius(), ball.getRadius()); // Calculate the center of the ball
    center += ball.velocity * deltaTime;

    // Apply friction to the velocity
    // ball.velocity = (1 - (1 - friction) * deltaTime) * ball.velocity;

    ball.trail.push_front(ball.getPosition());
    if (ball.trail.size() > 0) // Limit the trail length to 100
    {
        ball.trail.pop_back();
    }

    // Check for collision with the boundary
    if (center.x - ball.getRadius() < boundary.left)
    {
        center.x = boundary.left + ball.getRadius();
        ball.setRadius(ball.getRadius() + 1);
        ball.velocity.x *= -1;
    }
    else if (center.x + ball.getRadius() > boundary.left + boundary.width)
    {
        center.x = boundary.left + boundary.width - ball.getRadius();
        ball.setRadius(ball.getRadius() + 1);
        ball.velocity.x *= -1;
    }

    if (center.y - ball.getRadius() < boundary.top)
    {
        center.y = boundary.top + ball.getRadius();
        ball.setRadius(ball.getRadius() + 1);
        ball.velocity.y *= -1;
    }
    else if (center.y + ball.getRadius() > boundary.top + boundary.height)
    {
        center.y = boundary.top + boundary.height - ball.getRadius();
        ball.setRadius(ball.getRadius() + 1);
        ball.velocity.y *= -1;
    }

    pos = center - sf::Vector2f(ball.getRadius(), ball.getRadius()); // Update the position of the ball
    ball.setPosition(pos);
}

float currentHue = 0.0f;

void checkCollision(Ball &ball1, Ball &ball2, sf::SoundBuffer buffer)
{
    sf::Vector2f posDiff = ball1.getPosition() - ball2.getPosition();
    float distSq = posDiff.x * posDiff.x + posDiff.y * posDiff.y;
    float radiusSum = ball1.getRadius() + ball2.getRadius();
    if (distSq < radiusSum * radiusSum)
    {
        // The balls are colliding
        sf::Vector2f collisionNormal = posDiff / std::sqrt(distSq);
        sf::Vector2f relativeVelocity = ball1.velocity - ball2.velocity;
        float dotProd = relativeVelocity.x * collisionNormal.x + relativeVelocity.y * collisionNormal.y;

        // Update the velocities
        if (dotProd < 0.f)
        {
            sf::Vector2f collisionImpulse = collisionNormal * dotProd;
            ball1.velocity -= collisionImpulse;
            ball2.velocity += collisionImpulse;

            // Adjust the positions of the balls so they are not overlapping
            float overlap = radiusSum - std::sqrt(distSq);
            ball1.move(collisionNormal * overlap / 2.f);
            ball2.move(-collisionNormal * overlap / 2.f);

            // Increase the size of the balls
            ball1.setRadius(ball1.getRadius() + 1);
            ball2.setRadius(ball2.getRadius() + 1);

            // Change the color of the balls
            // thread_local std::default_random_engine generator(std::random_device{}());
            // thread_local std::uniform_real_distribution<float> distribution(0.0, 1.0);
            // float h = distribution(generator); // Random hue
            // ball1.setFillColor(hsvToRgb(h, 1, 1));
            // ball2.setFillColor(hsvToRgb(h, 1, 1));

            // // Change the color of the balls
            currentHue += 0.01f; // Increment the hue
            if (currentHue > 1.0f)
                currentHue -= 1.0f; // Wrap the hue back to 0 when it exceeds 1
            ball1.setFillColor(hsvToRgb(currentHue, 1, 1));
            ball2.setFillColor(hsvToRgb(currentHue, 1, 1));

            std::thread([buffer]()
                        {      
            sf::Sound sound;            
            sound.setBuffer(buffer);

            sound.play();

            // Keep the program running until the sound has finished playing
            while (sound.getStatus() == sf::Sound::Playing)
            {
                sf::sleep(sf::milliseconds(100));
            }

            return 0; })
                .detach();
        }
    }
}

sf::Vector2f gravity(0, 9.8); // Define a gravity vector

float timeScale = 1.f / 10.f; // Define a time scale factor. 1.0f is real-time, < 1.0f is slower, > 1.0f is faster

int main()
{
    sf::RenderWindow window(sf::VideoMode(1600, 1000), "Physics Simulation");

    sf::SoundBuffer buffer;
    if (!buffer.loadFromFile("C:\\Users\\jaram\\BallSiumlation\\VerletIntegration\\bounce.wav"))
    {
        std::cerr << "Failed to load sound file\n";
        return 1;
    }

    // Define the boundary
    sf::FloatRect boundary(0, 0, 1600, 1000);

    sf::Vector2f circleCenter(boundary.left + boundary.width / 2, boundary.top + boundary.height / 2);
    float circleRadius = boundary.width / 3.3;

    // Create a rectangle that represents the border
    sf::RectangleShape border(sf::Vector2f(boundary.width, boundary.height));
    border.setPosition(boundary.left, boundary.top);
    border.setFillColor(sf::Color::Transparent);
    border.setOutlineThickness(5);
    border.setOutlineColor(sf::Color::White);

    // Create a circle shape for the boundary
    sf::CircleShape boundaryShape(circleRadius);
    boundaryShape.setPointCount(100);                         // Increase the point count for better resolution
    boundaryShape.setFillColor(sf::Color(255, 255, 255, 90)); // Semi-transparent white fill
    boundaryShape.setOutlineColor(sf::Color::White);          // Red outline
    boundaryShape.setOutlineThickness(5);                     // Thicker outline
    boundaryShape.setPosition(circleCenter.x - circleRadius, circleCenter.y - circleRadius);

    srand(static_cast<unsigned int>(time(0))); // Seed the random number generator

    std::vector<Ball> balls;
    for (int i = 0; i < 2; ++i)
    {
        Ball ball(30);
        float angle = static_cast<float>(rand()) / RAND_MAX * 2 * 3.14159265358979323846;                 // Random angle
        float distance = sqrt(static_cast<float>(rand()) / RAND_MAX) * (circleRadius - ball.getRadius()); // Random distance
        float x = circleCenter.x + distance * cos(angle);
        float y = circleCenter.y + distance * sin(angle);
        ball.setPosition(x, y);
        ball.velocity = sf::Vector2f(0, 0);
        balls.push_back(ball);
    }

    std::vector<std::thread> threads;
    int numThreads = std::thread::hardware_concurrency(); // Get the number of threads supported by the hardware
    for (int i = 0; i < numThreads; ++i)
    {
        threads.push_back(std::thread());
    }

    sf::Clock clock;
    while (window.isOpen())
    {
        sf::Event event;
        while (window.pollEvent(event))
        {
            if (event.type == sf::Event::Closed)
                window.close();
        }

        float deltaTime = clock.restart().asSeconds() * timeScale; // Apply the time scale factor

        int ballsPerThread = balls.size() / numThreads;

        for (int i = 0; i < numThreads; ++i)
        {
            if (threads[i].joinable())
            {
                threads[i].join(); // Wait for the thread to finish if it's still running
            }

            // Assign a chunk of the balls vector to each thread
            threads[i] = std::thread([&balls, i, ballsPerThread, &boundary, &circleCenter, &circleRadius, deltaTime, numThreads, &buffer]()
                                     {
            int start = i * ballsPerThread;
            int end = (i + 1) * ballsPerThread;
            if (i == numThreads - 1) {
                end = balls.size(); // Make sure the last thread processes all remaining balls
            }

            for (int j = start; j < end; ++j) {
                balls[j].velocity += gravity; // Apply gravity to each ball
                verlet(balls[j], deltaTime, boundary);

                sf::Vector2f center = balls[j].getPosition() + sf::Vector2f(balls[j].getRadius(), balls[j].getRadius());
                sf::Vector2f diff = center - circleCenter;
                float distance = std::sqrt(diff.x * diff.x + diff.y * diff.y);

                if (distance + balls[j].getRadius() > circleRadius)
                {
                    // The ball is outside the boundary
                    // Reflect the ball's velocity and adjust its position
                    diff /= distance; // Normalize the difference vector
                    balls[j].velocity -= 2 * (balls[j].velocity.x * diff.x + balls[j].velocity.y * diff.y) * diff;
                    center = circleCenter + diff * (circleRadius - balls[j].getRadius());
                    balls[j].setPosition(center.x - balls[j].getRadius(), center.y - balls[j].getRadius());
                    
                     std::thread([&balls, j, &circleRadius]()
                        { 
                    // Wait for a few milliseconds
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
                    if (balls[j].getRadius() >= circleRadius)
                    {
                        balls[j].setRadius(balls[j].getRadius() + 0);
                    }
                    else
                        balls[j].setRadius(balls[j].getRadius() + 1);
                    
                       return 0; })
                        .detach();

                    std::thread([buffer, j , &balls, &circleRadius]()
                        {    
                    sf::Sound sound;            
                    sound.setBuffer(buffer);

                    if (balls[j].getRadius() >= circleRadius)
                    {

                    }
                    else
                    sound.play();

                    // Keep the program running until the sound has finished playing
                    while (sound.getStatus() == sf::Sound::Playing)
                    {
                        sf::sleep(sf::milliseconds(100));
                    }
                    
                    return 0; })
                        .detach();


                }

                // Check for collisions between the balls
                for (size_t k = j + 1; k < balls.size(); ++k)
                {
                    checkCollision(balls[j], balls[k], buffer);
                }

                balls[j].move(balls[j].velocity * deltaTime); // Move the ball
            } });
        }

        for (auto &thread : threads)
        {
            thread.join(); // Wait for all threads to finish
        }
        window.clear();
        window.draw(border);        // Draw the border
        window.draw(boundaryShape); // Draw the boundary

        for (const Ball &ball : balls)
        {
            float ch = 0;
            for (const sf::Vector2f &position : ball.trail)
            {
                sf::CircleShape afterImage(ball.getRadius());
                afterImage.setPosition(position);

                // Increment the hue and wrap it back to 0 when it exceeds 1
                ch += 0.005f;
                // currentHue *= 1.001f;
                if (ch > 1.0f) ch -= 1.0f;

                // Use the hue to set the color of the after image
                afterImage.setFillColor(hsvToRgb(ch, 1, 1));
                window.draw(afterImage);
            }
            window.draw(ball);
        }
        window.display();
    }

    return 0;
}