#include <iostream>
#include <cstdio>
#include <memory>
#include <stdexcept>
#include <array>
#include <iterator>
#include <regex>
#include <string>
#include <opencv2/opencv.hpp>
#include <filesystem>
#include <giomm/settings.h>
#include <signal.h>
#include <vector>

namespace fs = std::filesystem;
using namespace std;

/**
 * @brief Struct for storing Screen data
 */
struct Screen
{
    int x;
    int y;
    int width;
    int height;
};

#define OUTPUT "/tmp/wallpaper.png"

bool running = true;

// Regex for extracting screen data from xrandr
const std::regex SCREEN_REGEX("(.+?) connected (?:primary ){0,1}([0-9]+)x([0-9]+)\\+([0-9]+)\\+([0-9]+)",
                              std::regex_constants::multiline);

/**
 * @brief Scales an image to match the resolution of a screen
 *
 * @param input The input image to scale
 * @param screen The screen to match
 * @return cv::Mat
 */
cv::Mat scaleImage(cv::Mat &input, const Screen &screen)
{
    if (input.size().width != screen.width)
    {
        cv::Mat resized(screen.width, screen.height, CV_8UC3, cv::Scalar(0, 0, 0));
        cv::resize(input, resized, cv::Size(screen.width, screen.height));
        return resized;
    }
    return input;
}

/**
 * @brief Crops an image to match a screens aspect ratio
 * It crops by zooming
 *
 * @param input The input image to crop
 * @param screen The screen to match
 * @return cv::Mat
 */
cv::Mat cropImage(cv::Mat &input, const Screen &screen)
{
    auto imageSize = input.size();
    double expectedRatio = (double)screen.width / screen.height;

    double imageRatio = (double)imageSize.width / imageSize.height;

    if (expectedRatio == imageRatio)
    {
        return input;
    }

    if (expectedRatio < imageRatio)
    {
        int correctedWidth = (double)imageSize.height * expectedRatio;
        cv::Mat corrected = input(cv::Rect((imageSize.width - correctedWidth) >> 1, 0, correctedWidth, imageSize.height));
        return corrected;
    }
    int correctedHeight = (double)imageSize.width / expectedRatio;
    cv::Mat corrected = input(cv::Rect(0, (imageSize.height - correctedHeight) >> 1, imageSize.width, correctedHeight));

    return corrected;
}

/**
 * @brief Builds a collage of backdrops using random photos
 * from a source directory that'll span across a set of screens
 *
 * @param screens The set of screens to make the backdrop for
 * @param directory Source directory
 * @return cv::Mat
 */
cv::Mat buildBackdrop(std::vector<Screen> &screens, string directory)
{
    srand(time(0));

    vector<string> pickedImages(screens.size());
    vector<string> images;

    for (const auto &entry : fs::directory_iterator(directory))
        images.push_back(entry.path());

    int maxHeight = 0;
    int maxWidth = 0;

    int i = 0;

    for (const auto &screen : screens)
    {
        maxHeight = max(maxHeight, screen.height + screen.y);
        maxWidth = max(maxWidth, screen.width + screen.x);
        pickedImages.at(i++) = images.at(rand() % images.size());
    }

    cv::Mat canvas(maxHeight, maxWidth, CV_8UC3, cv::Scalar(0, 0, 0));

    i = 0;
    for (const auto &screen : screens)
    {
        auto input = pickedImages.at(i++);
        cv::Mat image = cv::imread(input);

        image = cropImage(image, screen);
        image = scaleImage(image, screen);
        image.copyTo(canvas(cv::Rect(screen.x, screen.y, screen.width, screen.height)));
    }

    cv::imwrite(OUTPUT, canvas);

    return canvas;
}

/**
 * @brief  Execute a system command and returns the results
 *
 * @param cmd
 * @return std::string
 */
std::string execute(const char *cmd)
{

    std::array<char, 128> buffer;
    std::string result;

    FILE *pipe = popen(cmd, "r");

    if (!pipe)
    {
        throw std::runtime_error("popen() failed. Exiting");
    }
    while (fgets(buffer.data(), static_cast<int>(buffer.size()), pipe) != nullptr)
    {
        result += buffer.data();
    }

    pclose(pipe);

    return result;
}

/**
 * @brief Configure GSettings for to handle our new wallpaper
 */
bool configureGSettings()
{
    Glib::RefPtr<Gio::Settings> backgroundSetting = Gio::Settings::create("org.gnome.desktop.background");
    bool settingResult = backgroundSetting->set_string("picture-uri-dark", OUTPUT);

    if (!settingResult)
    {
        printf("Failed to set wallpaper\n");
        return false;
    }

    settingResult = backgroundSetting->set_string("picture-options", "spanned");

    if (!settingResult)
    {
        printf("Failed to set wallpaper\n");
        return false;
    }
    return true;
}

/**
 * @brief Loads screens position and resolution from xrandr
 *
 * @return vector<Screen>
 */
vector<Screen> loadScreens()
{
    string xrandResult = execute("xrandr");

    if (!std::regex_search(xrandResult, SCREEN_REGEX))
    {
        printf("xrandr failed to get expected results.");
        return vector<Screen>();
    }

    auto begin = std::sregex_iterator(xrandResult.begin(), xrandResult.end(), SCREEN_REGEX);
    auto end = std::sregex_iterator();

    vector<Screen> screens;
    for (std::sregex_iterator i = begin; i != end; ++i)
    {
        std::smatch matches = *i;

        screens.push_back({.x = stoi(matches[4].str()),
                           .y = stoi(matches[5].str()),
                           .width = stoi(matches[2].str()),
                           .height = stoi(matches[3].str())});
    }
    return screens;
}

int main(int argc, char *argv[])
{
    char source[256];
    int time;

    if (argc > 1)
    {
        sprintf(source, "%s", argv[1]);
    }
    else
    {
        printf("Cannot run without a supplied directory for a source of wallpapers. Exiting\n");
        exit(1);
    }

    if (argc > 2)
    {
        time = stoi(argv[2]);
    }
    else
    {
        time = 60;
    }

    vector<Screen> screens = loadScreens();

    if (screens.size() == 0)
    {
        printf("Failed to load screens. Exiting.");
        return 0;
    }

    if (!configureGSettings())
    {
        printf("Failed to set GSettings background settings. Exiting.");
        return 0;
    }

    signal(SIGINT, [](int signal)
           { running = false; });

    while (running)
    {
        buildBackdrop(screens, source);
        sleep(time);
    }

    return 0;
}