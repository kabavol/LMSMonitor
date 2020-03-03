
void parse_payload(const char* data)
{
  printf("the data >>>>>>>>>>>>>>>>>>>\n%s\n",data);
  // TODO: implementation details
}


void on_sse_event(char** headers, const char* data)
{
    // parse mesage payload - simple JSON - keep it light-weight
    parse_payload(data);
    visualize(); // go visualize (if screen active)

}
