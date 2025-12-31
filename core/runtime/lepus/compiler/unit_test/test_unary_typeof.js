let $dataProcessorMap = {"func": function (data) { return data + 1; }};
let processData;
processData = function (data, processorName) {
  let func = $dataProcessorMap["default"];
  if (processorName) {
    func = $dataProcessorMap[processorName];
  }
  if (typeof func === "function") {
    data = func(data);
  }
  return data;
};

Assert(processData(1, "func") == 2);
