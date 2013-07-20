using Assertions;

public class ClickTestCase
{
    public static bool run_with_timeout (MainLoop ml, uint timeout_ms)
    {
        bool timeout_reached = false;
        var t_id = Timeout.add (timeout_ms, () => {
            timeout_reached = true;
            debug ("Timeout reached");
            ml.quit ();
            return false;
        });

        ml.run ();

        if (!timeout_reached) Source.remove (t_id);

        return !timeout_reached;
    }

    public static void test_parse_search_result ()
    {
        try {
            var apps = new AvailableApps.from_json (FAKE_JSON_SEARCH_RESULT);
            assert_cmpuint (apps.size, OperatorType.EQUAL, 3);
        } catch (GLib.Error e)
        {
            assert_not_reached ();
        }
    }

    public static void test_parse_search_result_item ()
    {
        try {
            var app = new AvailableApps.from_json (FAKE_JSON_SEARCH_RESULT)[1];
            assert_cmpstr (app.app_id, OperatorType.EQUAL, "org.example.fantastiqueapp");
            assert_cmpstr (app.title, OperatorType.EQUAL, "Fantastic App");
            assert_cmpstr (app.price, OperatorType.EQUAL, "0");
            assert_cmpstr (app.icon_url, OperatorType.EQUAL, "http://assets.ubuntu.com/sites/ubuntu/504/u/img/ubuntu/features/icon-find-more-apps-64x64.png");
        } catch (GLib.Error e)
        {
            assert_not_reached ();
        }
    }

    public static void test_parse_app_details ()
    {
        try {
            var details = new AppDetails.from_json (FAKE_JSON_PACKAGE_DETAILS);
            assert_cmpuint (details.binary_filesize, OperatorType.EQUAL, 123456);
            assert_cmpstr (details.keywords[1], OperatorType.EQUAL, "fantastic application");
            assert_cmpstr (details.description, OperatorType.EQUAL, "This application does magic.");
            assert_cmpstr (details.main_screenshot_url, OperatorType.EQUAL,
                "http://software-center.ubuntu.com/site_media/appmedia/2012/09/Screen_03.jpg");
            assert_cmpstr (details.more_screenshot_urls[1], OperatorType.EQUAL,
                "http://software-center.ubuntu.com/site_media/appmedia/2012/09/Screen_02.jpg");
            assert_cmpstr (details.download_url, OperatorType.EQUAL,
                "http://software-center.ubuntu.com/org.example.full_app/full_app-0.1.1.tar.gz");
        } catch (GLib.Error e)
        {
            assert_not_reached ();
        }
    }

    public static void test_download_manager ()
    {
        HashTable<string, string> credentials = new HashTable<string, string> (str_hash, str_equal);
        credentials["consumer_key"] = "...";
        credentials["consumer_secret"] = "...";
        credentials["token"] = "...";
        credentials["token_secret"] = "...";

        var sd = new SignedDownload (credentials);
        //var server = "https://public.apps.staging.ubuntu.com";
        //var path = "/download/ar.com.beuno/wheather-touch/ar.com.beuno.wheather-touch-0.2";
        //var url = server + path;
        var url = "http://alecu.com.ar/test/click/demo.php";


        Download download = null;

        MainLoop mainloop = new MainLoop ();
        sd.start_download.begin(url, (obj, res) => {
            mainloop.quit ();
            try {
                download = sd.start_download.end (res);
            } catch (GLib.Error e) {
                error ("Can't start download: %s", e.message);
            }
        });
        assert (run_with_timeout (mainloop, 10000));

        debug ("actually starting download");
    }

    public static void test_fetch_credentials ()
    {
        MainLoop mainloop = new MainLoop ();
        var sd = new SignedDownload (null);
        Download download = null;
        var url = "asdf";

        sd.start_download.begin(url, (obj, res) => {
            mainloop.quit ();
            try {
                download = sd.start_download.end (res);
            } catch (GLib.Error e) {
                error ("Can't start download: %s", e.message);
            }
        });
        assert (run_with_timeout (mainloop, 10000));
    }

    public static int main (string[] args)
    {
        Test.init (ref args);
        Test.add_data_func ("/Unit/ClickChecker/Test_Parse_Search_Result", test_parse_search_result);
        Test.add_data_func ("/Unit/ClickChecker/Test_Parse_Search_Result_Item", test_parse_search_result_item);
        Test.add_data_func ("/Unit/ClickChecker/Test_Parse_App_Details", test_parse_app_details);
        Test.add_data_func ("/Unit/ClickChecker/Test_Download_Manager", test_download_manager);
        Test.add_data_func ("/Unit/ClickChecker/Test_Fetch_Credentials", test_fetch_credentials);
        return Test.run ();
    }
}
