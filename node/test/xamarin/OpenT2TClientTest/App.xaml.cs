using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

using Xamarin.Forms;

namespace OpenT2T.Test
{
    public partial class App : Application
    {
        TestLogView testLogView;

        public App()
        {
            InitializeComponent();

            Label titleLabel = new Label
            {
                HorizontalOptions = LayoutOptions.Center,
                Text = "OpenT2T Xamarin Tests",
                FontSize = 24,
                TextColor = Color.Black,
            };

            Button testButton = new Button
            {
                HorizontalOptions = LayoutOptions.Center,
                Text = "Run Tests",
                TextColor = Color.White,
                FontAttributes = FontAttributes.Bold,
                BorderColor = Color.Black,
                BorderRadius = 4,
                BorderWidth = 1,
                WidthRequest = 100,
                BackgroundColor = Color.FromRgb(160, 160, 255),
            };
            testButton.Clicked += this.OnTestButtonClicked;

            testLogView = new TestLogView
            {
                VerticalOptions = LayoutOptions.FillAndExpand,
            };

            StackLayout headerPanel = new StackLayout
            {
                Padding = new Thickness(0, 30, 0, 20),
                Spacing = 15,
                Children =
                {
                    titleLabel,
                    testButton,
                },
                BackgroundColor = Color.FromRgb(208, 208, 255),
            };

            this.MainPage = new ContentPage
            {
                Content = new StackLayout
                {
                    VerticalOptions = LayoutOptions.FillAndExpand,
                    Spacing = 0,
                    Children =
                    {
                        headerPanel,
                        testLogView,
                    }
                }
            };
        }

        private void OnTestButtonClicked(object sender, EventArgs e)
        {
            Tests tests = new Tests(this.testLogView);
            tests.Run();
        }

        protected override void OnStart()
        {
            // Handle when your app starts
        }

        protected override void OnSleep()
        {
            // Handle when your app sleeps
        }

        protected override void OnResume()
        {
            // Handle when your app resumes
        }
    }
}
