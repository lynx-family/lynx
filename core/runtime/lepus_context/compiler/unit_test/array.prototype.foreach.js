let items = ['item1', 'item2', 'item3'];
let copy = [];

// before
let len = items.length;
for (let i = 0; i < len; i++) {
  items.push(items[i]);
}
Assert(items == ['item1', 'item2', 'item3', 'item1', 'item2', 'item3']);

// after
items.forEach(function(item) {
  items.push(item);
});

Assert(
  items == ['item1', 'item2', 'item3', 'item1', 'item2', 'item3', 'item1', 'item2', 'item3', 'item1', 'item2', 'item3']
);
