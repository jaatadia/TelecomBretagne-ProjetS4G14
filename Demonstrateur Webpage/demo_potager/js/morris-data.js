$(function() {
    var max = 30;
    var min = 10;
    Morris.Area({
        element: 'morris-area-chart',
        data: [{
            time: '0',
            max: max,
            temp: 20,
            min: min
        }, {
            time: '1',
            max: max,
            temp: 20,
            min: min
        }, {
            time: '2',
            max: max,
            temp: 20,
            min: min
        }, {
            time: '3',
            max: max,
            temp: 20,
            min: min
        }, {
            time: '4',
            max: max,
            temp: 20,
            min: min
        }, {
            time: '5',
            max: max,
            temp: 20,
            min: min
        }, {
            time: '6',
            max: max,
            temp: 20,
            min: min
        }, {
            time: '7',
            max: max,
            temp: 20,
            min: min
        }, {
            time: '8',
            max: max,
            temp: 20,
            min: min
        }, {
            time: '9',
            max: max,
            temp: 20,
            min: min
        }],
        xkey: 'time',
        ykeys: ['max', 'temp', 'min'],
        labels: ['max', 'temp', 'min'],
        resize: true
    });
});
