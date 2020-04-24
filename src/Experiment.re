let key = "bucket_hash";
let bucketHash =
  ref(
    (Dom.Storage.localStorage |> Dom.Storage.getItem(key))
    ->Belt.Option.flatMap(int_of_string_opt),
  );

let triggerKey = "triggered_experiments";
let triggeredMap =
  ref(
    (Dom.Storage.localStorage |> Dom.Storage.getItem(triggerKey))
    ->Belt.Option.map(value => {
        let json = Js.Json.parseExn(value);
        Json.Decode.(dict(string, json));
      })
    ->Belt.Option.getWithDefault(Js.Dict.empty()),
  );
let addTrigger = (key, value) => {
  (triggeredMap^)->Js.Dict.set(key, value);
  Dom.Storage.(
    localStorage
    |> setItem(
         triggerKey,
         Js.Json.stringify(Json.Encode.(dict(string, triggeredMap^))),
       )
  );
};

exception UnexpectedExperimentId(string);
let getBucketHash = () => {
  switch (bucketHash^) {
  | Some(bucketHash) => bucketHash
  | None =>
    let value = Js.Math.random_int(0, max_int);
    bucketHash := Some(value);
    Dom.Storage.localStorage
    |> Dom.Storage.setItem(key, string_of_int(value));
    value;
  };
};

module ExperimentIds = {
  let matchListNotice = "524";
};

let getBucketIdForExperiment = (~experimentId) =>
  if (experimentId == ExperimentIds.matchListNotice) {
    string_of_int(
      // skip two low bits for now
      // Use the 3rd bit (50%)
      getBucketHash() lsr 2 land 1,
    );
  } else {
    raise(UnexpectedExperimentId(experimentId));
  };

let trigger = (~experimentId, ~bucketId) =>
  if ((triggeredMap^)->Js.Dict.get(experimentId) != Some(bucketId)) {
    addTrigger(experimentId, bucketId);
    Analytics.Amplitude.addExperimentBucket(~experimentId, ~bucketId);
    Analytics.Amplitude.logEventWithProperties(
      ~eventName="Experiment Triggered",
      ~eventProperties={"experimentId": experimentId, "bucketId": bucketId},
    );
  };