import 'package:flutter/material.dart';

void main() {
  runApp(const MyApp());
}

class MyApp extends StatelessWidget {
  const MyApp({super.key});

  // This widget is the root of your application.
  @override
  Widget build(BuildContext context) {
    return MaterialApp(
      title: 'WebView Demo',
      theme: ThemeData(
        // This is the theme of your application.
        //
        // TRY THIS: Try running your application with "flutter run". You'll see
        // the application has a purple toolbar. Then, without quitting the app,
        // try changing the seedColor in the colorScheme below to Colors.green
        // and then invoke "hot reload" (save your changes or press the "hot
        // reload" button in a Flutter-supported IDE, or press "r" if you used
        // the command line to start the app).
        //
        // Notice that the counter didn't reset back to zero; the application
        // state is not lost during the reload. To reset the state, use hot
        // restart instead.
        //
        // This works for code too, not just values: Most code changes can be
        // tested with just a hot reload.
        colorScheme: ColorScheme.fromSeed(seedColor: Colors.deepPurple),
        useMaterial3: true,
      ),
      home: const MyHomePage(title: 'WebView Demo'),
    );
  }
}

class MyHomePage extends StatefulWidget {
  const MyHomePage({super.key, required this.title});

  // This widget is the home page of your application. It is stateful, meaning
  // that it has a State object (defined below) that contains fields that affect
  // how it looks.

  // This class is the configuration for the state. It holds the values (in this
  // case the title) provided by the parent (in this case the App widget) and
  // used by the build method of the State. Fields in a Widget subclass are
  // always marked "final".

  final String title;

  @override
  State<MyHomePage> createState() => _MyHomePageState();
}

int kMaxFlex = 12;

class _MyHomePageState extends State<MyHomePage> {
  int flex = 5;
  late Widget view;
  TextEditingController c1 = TextEditingController(text: 'lorem...');
  TextEditingController c2 = TextEditingController(text: 'ipsum...');

  void _incrementCounter() {
    setState(() {
      // This call to setState tells the Flutter framework that something has
      // changed in this State, which causes it to rerun the build method below
      // so that the display can reflect the updated values. If we changed
      // _counter without calling setState(), then the build method would not be
      // called again, and so nothing would appear to happen.
      if (flex < kMaxFlex) flex++;
    });
  }

  void _decrementCounter() {
    setState(() {
      if (flex > 0) flex--;
    });
  }

  void _buildPlatformView() {
    view = Win32View(viewType: "test");
  }

  @override
  void initState() {
    super.initState();
    _buildPlatformView();
  }

  @override
  Widget build(BuildContext context) {
    // This method is rerun every time setState is called, for instance as done
    // by the _incrementCounter method above.
    //
    // The Flutter framework has been optimized to make rerunning build methods
    // fast, so that you can just rebuild anything that needs updating rather
    // than having to individually change instances of widgets.
    return Scaffold(
      appBar: AppBar(
        // TRY THIS: Try changing the color here to a specific color (to
        // Colors.amber, perhaps?) and trigger a hot reload to see the AppBar
        // change color while the other colors stay the same.
        backgroundColor: Theme.of(context).colorScheme.inversePrimary,
        // Here we take the value from the MyHomePage object that was created by
        // the App.build method, and use it to set our appbar title.
        title: Text(widget.title),
      ),
      body: Center(
        // Center is a layout widget. It takes a single child and positions it
        // in the middle of the parent.
        child: Column(
          // Column is also a layout widget. It takes a list of children and
          // arranges them vertically. By default, it sizes itself to fit its
          // children horizontally, and tries to be as tall as its parent.
          //
          // Column has various properties to control how it sizes itself and
          // how it positions its children. Here we use mainAxisAlignment to
          // center the children vertically; the main axis here is the vertical
          // axis because Columns are vertical (the cross axis would be
          // horizontal).
          //
          // TRY THIS: Invoke "debug painting" (choose the "Toggle Debug Paint"
          // action in the IDE, or press "p" in the console), to see the
          // wireframe for each widget.
          mainAxisAlignment: MainAxisAlignment.center,
          children: <Widget>[
            TextField(
              controller: c1,
            ),
            Stack(
              children: [
                ConstrainedBox(
                  constraints: BoxConstraints(maxHeight: 300),
                  child: Row(
                    children: [
                      Expanded(
                        flex: flex,
                        child: view,
                      ),
                      Expanded(
                        flex: kMaxFlex - flex,
                        child: Container(color: Colors.green),
                      )
                    ],
                  ),
                ),
                Container(
                  decoration: BoxDecoration(
                    border: Border.all(color: Colors.red),
                    borderRadius: const BorderRadius.all(Radius.circular(50)),
                    color: Colors.red,
                  ),
                  width: 300,
                  height: 300,
                  //color: Colors.red,
                  // Build and run me, then comment out the above line, then hot restart.
                ),
              ],
            ),
            TextField(
              controller: c2,
            ),
            // Stack(
            //   children: [
            //     Focus(
            //       child: Container(width: 100, height: 100, color: Colors.pink),
            //       onFocusChange: (focus) {if (focus) print('Focus 1!');},
            //     ),
            //     Focus(
            //       child: Container(width: 100, height: 100, color: Colors.green),
            //       onFocusChange: (focus) {if (focus) print('Focus 2!');},
            //     ),
            //     Focus(
            //       child: Container(width: 100, height: 100, color: Colors.yellow),
            //       onFocusChange: (focus) {if (focus) print('Focus 3!');},
            //     ),
            //   ]
            // ),
            FloatingActionButton(
              onPressed: _incrementCounter,
              tooltip: 'Widen WebView',
              child: const Icon(Icons.add),
            ),
            FloatingActionButton(
              onPressed: _decrementCounter,
              tooltip: 'Widen WebView',
              child: const Icon(Icons.minimize),
            ),
          ],
        ),
      ),
      floatingActionButton: FloatingActionButton(
        onPressed: () {
          setState((){
            flex = 5;
          });
        },
        tooltip: 'Widen WebView',
        child: const Icon(Icons.refresh),
      ), // This trailing comma makes auto-formatting nicer for build methods.
    );
  }
}
